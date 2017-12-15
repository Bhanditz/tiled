/*
 * worldmanager.cpp
 * Copyright 2017, Thorbjørn Lindeijer <bjorn@lindeijer.nl>
 *
 * This file is part of libtiled.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "worldmanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <QDebug>

namespace Tiled {

WorldManager *WorldManager::mInstance;

WorldManager::WorldManager()
{
}

WorldManager::~WorldManager()
{
    qDeleteAll(mWorlds);
}

WorldManager &WorldManager::instance()
{
    if (!mInstance)
        mInstance = new WorldManager;

    return *mInstance;
}

void WorldManager::deleteInstance()
{
    delete mInstance;
    mInstance = nullptr;
}

void WorldManager::loadWorld(const QString &fileName)
{
    unloadWorld(fileName);  // unload possibly existing world

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QJsonParseError error;
    const QJsonObject object = QJsonDocument::fromJson(file.readAll(), &error).object();
    if (error.error != QJsonParseError::NoError)
        return;

    QDir dir = QFileInfo(fileName).dir();
    QScopedPointer<World> world(new World);

    const QJsonArray maps = object.value(QLatin1String("maps")).toArray();
    for (const QJsonValue &value : maps) {
        const QJsonObject mapObject = value.toObject();

        World::MapEntry map;
        map.fileName = QDir::cleanPath(dir.filePath(mapObject.value(QLatin1String("fileName")).toString()));
        map.position.setX(mapObject.value(QLatin1String("x")).toInt());
        map.position.setY(mapObject.value(QLatin1String("y")).toInt());

        world->maps.append(map);
    }

    const QJsonArray patterns = object.value(QLatin1String("patterns")).toArray();
    for (const QJsonValue &value : patterns) {
        const QJsonObject patternObject = value.toObject();

        World::Pattern pattern;
        pattern.regexp.setPattern(patternObject.value(QLatin1String("regexp")).toString());
        pattern.multiplier = patternObject.value(QLatin1String("multiplier")).toInt(1);

        if (pattern.regexp.captureCount() == 2)
            qWarning() << "Invalid number of captures in" << pattern.regexp;
        else if (pattern.multiplier <= 0)
            qWarning() << "Invalid multiplier:" << pattern.multiplier;
        else
            world->patterns.append(pattern);
    }

    mWorlds.insert(fileName, world.take());
}

void WorldManager::unloadWorld(const QString &fileName)
{
    delete mWorlds.take(fileName);
}

const World *WorldManager::worldForMap(const QString &fileName) const
{
    for (const World *world : mWorlds) {
        for (const World::MapEntry &mapEntry : world->maps) {
            // todo: support matching by patterns
            if (mapEntry.fileName == fileName)
                return world;
        }
    }
    return nullptr;
}

} // namespace Tiled
