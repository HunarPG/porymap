#include "history.h"
#include "map.h"
#include "imageproviders.h"
#include "scripting.h"

#include "editcommands.h"

#include <QTime>
#include <QPainter>
#include <QImage>
#include <QRegularExpression>


Map::Map(QObject *parent) : QObject(parent)
{
    editHistory.setClean();
}

Map::~Map() {
    // delete all associated events
    while (!ownedEvents.isEmpty()) {
        Event *last = ownedEvents.takeLast();
        if (last) delete last;
    }
}

void Map::setName(QString mapName) {
    name = mapName;
    constantName = mapConstantFromName(mapName);
    scriptsLoaded = false;
}

void Map::setLayout(Layout *layout) {
    this->layout = layout;
    if (layout) {
        this->layoutId = layout->id;
    }
}

QString Map::mapConstantFromName(QString mapName, bool includePrefix) {
    // Transform map names of the form 'GraniteCave_B1F` into map constants like 'MAP_GRANITE_CAVE_B1F'.
    static const QRegularExpression caseChange("([a-z])([A-Z])");
    QString nameWithUnderscores = mapName.replace(caseChange, "\\1_\\2");
    const QString prefix = includePrefix ? projectConfig.getIdentifier(ProjectIdentifier::define_map_prefix) : "";
    QString withMapAndUppercase = prefix + nameWithUnderscores.toUpper();
    static const QRegularExpression underscores("_+");
    QString constantName = withMapAndUppercase.replace(underscores, "_");

    // Handle special cases.
    // SSTidal needs to be SS_TIDAL, rather than SSTIDAL
    constantName = constantName.replace("SSTIDAL", "SS_TIDAL");

    return constantName;
}

int Map::getWidth() {
    return layout->getWidth();
}

int Map::getHeight() {
    return layout->getHeight();
}

int Map::getBorderWidth() {
    return layout->getBorderWidth();
}

int Map::getBorderHeight() {
    return layout->getBorderHeight();
}

QPixmap Map::renderConnection(MapConnection connection, Layout * fromLayout) {
    int x, y, w, h;
    if (connection.direction == "up") {
        x = 0;
        y = getHeight() - BORDER_DISTANCE;
        w = getWidth();
        h = BORDER_DISTANCE;
    } else if (connection.direction == "down") {
        x = 0;
        y = 0;
        w = getWidth();
        h = BORDER_DISTANCE;
    } else if (connection.direction == "left") {
        x = getWidth() - BORDER_DISTANCE;
        y = 0;
        w = BORDER_DISTANCE;
        h = getHeight();
    } else if (connection.direction == "right") {
        x = 0;
        y = 0;
        w = BORDER_DISTANCE;
        h = getHeight();
    } else {
        // this should not happen
        x = 0;
        y = 0;
        w = getWidth();
        h = getHeight();
    }

    //render(true, fromLayout, QRect(x, y, w, h));
    //QImage connection_image = image.copy(x * 16, y * 16, w * 16, h * 16);
    return this->layout->render(true, fromLayout, QRect(x, y, w, h)).copy(x * 16, y * 16, w * 16, h * 16);
    //return QPixmap::fromImage(connection_image);
}

void Map::openScript(QString label) {
    emit openScriptRequested(label);
}

QList<Event *> Map::getAllEvents() const {
    QList<Event *> all_events;
    for (const auto &event_list : events) {
        all_events << event_list;
    }
    return all_events;
}

QStringList Map::getScriptLabels(Event::Group group) {
    if (!this->scriptsLoaded) {
        this->scriptsFileLabels = ParseUtil::getGlobalScriptLabels(this->getScriptsFilePath());
        this->scriptsLoaded = true;
    }

    QStringList scriptLabels;

    // Get script labels currently in-use by the map's events
    if (group == Event::Group::None) {
        ScriptTracker scriptTracker;
        for (Event *event : this->getAllEvents()) {
            event->accept(&scriptTracker);
        }
        scriptLabels = scriptTracker.getScripts();
    } else {
        ScriptTracker scriptTracker;
        for (Event *event : events.value(group)) {
            event->accept(&scriptTracker);
        }
        scriptLabels = scriptTracker.getScripts();
    }

    // Add scripts from map's scripts file, and empty names.
    scriptLabels.append(this->scriptsFileLabels);
    scriptLabels.sort(Qt::CaseInsensitive);
    scriptLabels.prepend("0x0");
    scriptLabels.prepend("NULL");

    scriptLabels.removeAll("");
    scriptLabels.removeDuplicates();

    return scriptLabels;
}

QString Map::getScriptsFilePath() const {
    const bool usePoryscript = projectConfig.getUsePoryScript();
    auto path = QDir::cleanPath(QString("%1/%2/%3/scripts")
                                        .arg(projectConfig.getProjectDir())
                                        .arg(projectConfig.getFilePath(ProjectFilePath::data_map_folders))
                                        .arg(this->name));
    auto extension = Project::getScriptFileExtension(usePoryscript);
    if (usePoryscript && !QFile::exists(path + extension))
        extension = Project::getScriptFileExtension(false);
    path += extension;
    return path;
}

void Map::removeEvent(Event *event) {
    for (Event::Group key : events.keys()) {
        events[key].removeAll(event);
    }
}

void Map::addEvent(Event *event) {
    event->setMap(this);
    events[event->getEventGroup()].append(event);
    if (!ownedEvents.contains(event)) ownedEvents.append(event);
}

void Map::modify() {
    emit modified();
}

void Map::clean() {
    this->hasUnsavedDataChanges = false;
}

bool Map::hasUnsavedChanges() {
    return !editHistory.isClean() || !this->layout->editHistory.isClean() || hasUnsavedDataChanges || !isPersistedToFile;
}
