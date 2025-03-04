#include "tileseteditormetatileselector.h"
#include "imageproviders.h"
#include "project.h"
#include <QPainter>

TilesetEditorMetatileSelector::TilesetEditorMetatileSelector(Tileset *primaryTileset, Tileset *secondaryTileset, Layout *layout)
  : SelectablePixmapItem(32, 32, 1, 1) {
    this->setTilesets(primaryTileset, secondaryTileset, false);
    this->numMetatilesWide = 8;
    this->layout = layout;
    setAcceptHoverEvents(true);
    this->usedMetatiles.resize(Project::getNumMetatilesTotal());
}

int TilesetEditorMetatileSelector::numRows(int numMetatiles) {
    int numMetatilesHigh = numMetatiles / this->numMetatilesWide;
    if (numMetatiles % this->numMetatilesWide != 0) {
        // Round up height for incomplete last row
        numMetatilesHigh++;
    }
    return numMetatilesHigh;
}

int TilesetEditorMetatileSelector::numRows() {
    return this->numRows(this->primaryTileset->numMetatiles() + this->secondaryTileset->numMetatiles());
}

QImage TilesetEditorMetatileSelector::buildAllMetatilesImage() {
    return this->buildImage(0, this->primaryTileset->numMetatiles() + this->secondaryTileset->numMetatiles());
}

QImage TilesetEditorMetatileSelector::buildPrimaryMetatilesImage() {
    return this->buildImage(0, this->primaryTileset->numMetatiles());
}

QImage TilesetEditorMetatileSelector::buildSecondaryMetatilesImage() {
    return this->buildImage(Project::getNumMetatilesPrimary(), this->secondaryTileset->numMetatiles());
}

QImage TilesetEditorMetatileSelector::buildImage(int metatileIdStart, int numMetatiles) {
    int numMetatilesHigh = this->numRows(numMetatiles);
    int numPrimary = this->primaryTileset->numMetatiles();
    int maxPrimary = Project::getNumMetatilesPrimary();
    bool includesPrimary = metatileIdStart < maxPrimary;

    QImage image(this->numMetatilesWide * 32, numMetatilesHigh * 32, QImage::Format_RGBA8888);
    image.fill(Qt::magenta);
    QPainter painter(&image);
    for (int i = 0; i < numMetatiles; i++) {
        int metatileId = i + metatileIdStart;
        if (includesPrimary && metatileId >= numPrimary)
            metatileId += maxPrimary - numPrimary; // Skip over unused region of primary tileset
        QImage metatile_image = getMetatileImage(
                    metatileId,
                    this->primaryTileset,
                    this->secondaryTileset,
                    this->layout->metatileLayerOrder,
                    this->layout->metatileLayerOpacity,
                    true)
                .scaled(32, 32);
        int map_y = i / this->numMetatilesWide;
        int map_x = i % this->numMetatilesWide;
        QPoint metatile_origin = QPoint(map_x * 32, map_y * 32);
        painter.drawImage(metatile_origin, metatile_image);
    }
    painter.end();
    return image;
}

void TilesetEditorMetatileSelector::draw() {
    this->setPixmap(QPixmap::fromImage(this->buildAllMetatilesImage()));
    this->drawGrid();
    this->drawSelection();
    this->drawFilters();
}

bool TilesetEditorMetatileSelector::select(uint16_t metatileId) {
    if (!Tileset::metatileIsValid(metatileId, this->primaryTileset, this->secondaryTileset)) return false;
    QPoint coords = this->getMetatileIdCoords(metatileId);
    SelectablePixmapItem::select(coords.x(), coords.y(), 0, 0);
    this->selectedMetatile = metatileId;
    emit selectedMetatileChanged(metatileId);
    return true;
}

void TilesetEditorMetatileSelector::setTilesets(Tileset *primaryTileset, Tileset *secondaryTileset, bool draw) {
    this->primaryTileset = primaryTileset;
    this->secondaryTileset = secondaryTileset;

    if (draw) this->draw();
}

void TilesetEditorMetatileSelector::updateSelectedMetatile() {
    QPoint origin = this->getSelectionStart();
    uint16_t metatileId = this->getMetatileId(origin.x(), origin.y());
    if (Tileset::metatileIsValid(metatileId, this->primaryTileset, this->secondaryTileset))
        this->selectedMetatile = metatileId;
    else
        this->selectedMetatile = Project::getNumMetatilesPrimary() + this->secondaryTileset->numMetatiles() - 1;
    emit selectedMetatileChanged(this->selectedMetatile);
}

uint16_t TilesetEditorMetatileSelector::getSelectedMetatileId() {
    return this->selectedMetatile;
}

uint16_t TilesetEditorMetatileSelector::getMetatileId(int x, int y) {
    int index = y * this->numMetatilesWide + x;
    if (index < this->primaryTileset->numMetatiles()) {
        return static_cast<uint16_t>(index);
    } else {
        return static_cast<uint16_t>(Project::getNumMetatilesPrimary() + index - this->primaryTileset->numMetatiles());
    }
}

bool TilesetEditorMetatileSelector::shouldAcceptEvent(QGraphicsSceneMouseEvent *event) {
    QPoint pos = this->getCellPos(event->pos());
    return Tileset::metatileIsValid(getMetatileId(pos.x(), pos.y()), this->primaryTileset, this->secondaryTileset);
}

void TilesetEditorMetatileSelector::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (!shouldAcceptEvent(event)) return;
    SelectablePixmapItem::mousePressEvent(event);
    this->updateSelectedMetatile();
}

void TilesetEditorMetatileSelector::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (!shouldAcceptEvent(event)) return;
    SelectablePixmapItem::mouseMoveEvent(event);
    this->updateSelectedMetatile();
    emit hoveredMetatileChanged(this->selectedMetatile);
}

void TilesetEditorMetatileSelector::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (!shouldAcceptEvent(event)) return;
    SelectablePixmapItem::mouseReleaseEvent(event);
    this->updateSelectedMetatile();
}

void TilesetEditorMetatileSelector::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    QPoint pos = this->getCellPos(event->pos());
    uint16_t metatileId = this->getMetatileId(pos.x(), pos.y());
    emit this->hoveredMetatileChanged(metatileId);
}

void TilesetEditorMetatileSelector::hoverLeaveEvent(QGraphicsSceneHoverEvent*) {
    emit this->hoveredMetatileCleared();
}

QPoint TilesetEditorMetatileSelector::getMetatileIdCoords(uint16_t metatileId) {
    if (!Tileset::metatileIsValid(metatileId, this->primaryTileset, this->secondaryTileset))
    {
        // Invalid metatile id.
        return QPoint(0, 0);
    }
    int index = metatileId < Project::getNumMetatilesPrimary()
                ? metatileId
                : metatileId - Project::getNumMetatilesPrimary() + this->primaryTileset->numMetatiles();
    return QPoint(index % this->numMetatilesWide, index / this->numMetatilesWide);
}

QPoint TilesetEditorMetatileSelector::getMetatileIdCoordsOnWidget(uint16_t metatileId) {
    QPoint pos = getMetatileIdCoords(metatileId);
    pos.rx() = (pos.x() * this->cellWidth) + (this->cellWidth / 2);
    pos.ry() = (pos.y() * this->cellHeight) + (this->cellHeight / 2);
    return pos;
}

void TilesetEditorMetatileSelector::drawGrid() {
    if (!this->showGrid)
        return;

    QPixmap pixmap = this->pixmap();
    QPainter painter(&pixmap);
    const int numColumns = this->numMetatilesWide;
    const int numRows = this->numRows();
    for (int column = 1; column < numColumns; column++) {
        int x = column * 32;
        painter.drawLine(x, 0, x, numRows * 32);
    }
    for (int row = 1; row < numRows; row++) {
        int y = row * 32;
        painter.drawLine(0, y, numColumns * 32, y);
    }
    painter.end();
    this->setPixmap(pixmap);
}

void TilesetEditorMetatileSelector::drawFilters() {
    if (selectorShowUnused) {
        drawUnused();
    }
    if (selectorShowCounts) {
        drawCounts();
    }
}

void TilesetEditorMetatileSelector::drawUnused() {
    // setup the circle with a line through it image to layer above unused metatiles
    QPixmap redX(32, 32);
    redX.fill(Qt::transparent);

    QPen whitePen(Qt::white);
    whitePen.setWidth(1);
    QPen pinkPen(Qt::magenta);
    pinkPen.setWidth(1);

    QPainter oPainter(&redX);

    oPainter.setPen(whitePen);
    oPainter.drawEllipse(QRect(1, 1, 30, 30));
    oPainter.setPen(pinkPen);
    oPainter.drawEllipse(QRect(2, 2, 28, 28));
    oPainter.drawEllipse(QRect(3, 3, 26, 26));

    oPainter.setPen(whitePen);
    oPainter.drawEllipse(QRect(4, 4, 24, 24));

    whitePen.setWidth(5);
    oPainter.setPen(whitePen);
    oPainter.drawLine(0, 0, 31, 31);

    pinkPen.setWidth(3);
    oPainter.setPen(pinkPen);
    oPainter.drawLine(2, 2, 29, 29);

    oPainter.end();

    // draw symbol on unused metatiles
    QPixmap metatilesPixmap = this->pixmap();

    QPainter unusedPainter(&metatilesPixmap);
    unusedPainter.setOpacity(0.5);

    int primaryLength = this->primaryTileset->numMetatiles();
    int length_ = primaryLength + this->secondaryTileset->numMetatiles();

    for (int i = 0; i < length_; i++) {
        int tile = i;
        if (i >= primaryLength) {
            tile += Project::getNumMetatilesPrimary() - primaryLength;
        }
        if (!usedMetatiles[tile]) {
            unusedPainter.drawPixmap((i % 8) * 32, (i / 8) * 32, redX);
        }
    }

    unusedPainter.end();

    this->setPixmap(metatilesPixmap);
}

void TilesetEditorMetatileSelector::drawCounts() {
    QPen blackPen(Qt::black);
    blackPen.setWidth(1);

    QPixmap metatilesPixmap = this->pixmap();

    QPainter countPainter(&metatilesPixmap);
    countPainter.setPen(blackPen);

    for (int tile = 0; tile < this->usedMetatiles.size(); tile++) {
        int count = usedMetatiles[tile];
        QString countText = QString::number(count);
        if (count > 1000) countText = ">1k";
        countPainter.drawText((tile % 8) * 32, (tile / 8) * 32 + 32, countText);
    }

    // write in white and black for contrast
    QPen whitePen(Qt::white);
    whitePen.setWidth(1);
    countPainter.setPen(whitePen);

    int primaryLength = this->primaryTileset->numMetatiles();
    int length_ = primaryLength + this->secondaryTileset->numMetatiles();

    for (int i = 0; i < length_; i++) {
        int tile = i;
        if (i >= primaryLength) {
            tile += Project::getNumMetatilesPrimary() - primaryLength;
        }
        int count = usedMetatiles[tile];
        QString countText = QString::number(count);
        if (count > 1000) countText = ">1k";
        countPainter.drawText((i % 8) * 32 + 1, (i / 8) * 32 + 32 - 1, countText);
    }

    countPainter.end();

    this->setPixmap(metatilesPixmap);
}
