/****************************************************************************
**
** Copyright (c) 2009-2010, Jaco Naude
**
** This file is part of Qtilities which is released under the following
** licensing options.
**
** Option 1: Open Source
** Under this license Qtilities is free software: you can
** redistribute it and/or modify it under the terms of the GNU General
** Public License as published by the Free Software Foundation, either
** version 3 of the License, or (at your option) any later version.
**
** Qtilities is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Qtilities. If not, see http://www.gnu.org/licenses/.
**
** Option 2: Commercial
** Alternatively, this library is also released under a commercial license
** that allows the development of closed source proprietary applications
** without restrictions on licensing. For more information on this option,
** please see the project website's licensing page:
** http://www.qtilities.org/licensing.html
**
** If you are unsure which license is appropriate for your use, please
** contact support@qtilities.org.
**
****************************************************************************/

#include "SubjectTypeFilter.h"
#include "QtilitiesCoreConstants.h"
#include "Observer.h"

#include <Logger.h>

#include <QtDebug>
#include <QMutex>
#include <QVariant>

using namespace Qtilities::Core::Constants;

namespace Qtilities {
    namespace Core {
        FactoryItem<AbstractSubjectFilter, SubjectTypeFilter> SubjectTypeFilter::factory;
    }
}

struct Qtilities::Core::SubjectTypeFilterData {
    SubjectTypeFilterData() : inversed_filtering(false) {}

    bool inversed_filtering;
    QList<SubjectTypeInfo> known_subject_types;
    QString known_objects_group_name;
};

Qtilities::Core::SubjectTypeFilter::SubjectTypeFilter(const QString& known_objects_group_name, QObject* parent) : AbstractSubjectFilter(parent) {
    d = new SubjectTypeFilterData;
    d->known_objects_group_name = known_objects_group_name;
}

Qtilities::Core::AbstractSubjectFilter::EvaluationResult Qtilities::Core::SubjectTypeFilter::evaluateAttachment(QObject* obj) const {
    Q_UNUSED(obj)

    return AbstractSubjectFilter::Allowed;
}

bool Qtilities::Core::SubjectTypeFilter::initializeAttachment(QObject* obj, bool import_cycle) {
    Q_UNUSED(import_cycle)

    #ifndef QT_NO_DEBUG
        Q_ASSERT(observer != 0);
    #endif
    #ifdef QT_NO_DEBUG
        if (!obj)
            return false;
    #endif

    if (!observer) {
        LOG_TRACE("Cannot evaluate an attachment in a subject filter without an observer context.");
        return false;
    }

    bool is_known_type = false;

    // Check the obj meta info against the known filter types
    for (int i = 0; i < d->known_subject_types.count(); i++) {
        QString meta_type = d->known_subject_types.at(i).d_meta_type;
        if (obj->inherits(meta_type.toAscii().data())) {
            if (!d->inversed_filtering) {
                is_known_type = true;
                break;
            }
        } else {
            if (d->inversed_filtering) {
                is_known_type = true;
                break;
            }
        }
    }

    if (!is_known_type) {
        LOG_WARNING(QString(tr("Subject filter \"%1\" rejected attachment of object \"%2\" to observer \"%3\".")).arg(filterName()).arg(obj->objectName()).arg(observer->observerName()));
    }

    return is_known_type;
}

void Qtilities::Core::SubjectTypeFilter::finalizeAttachment(QObject* obj, bool attachment_successful, bool import_cycle) {
    Q_UNUSED(obj)
    Q_UNUSED(attachment_successful)
    Q_UNUSED(import_cycle)
}

Qtilities::Core::AbstractSubjectFilter::EvaluationResult Qtilities::Core::SubjectTypeFilter::evaluateDetachment(QObject* obj) const {
    Q_UNUSED(obj)

    return AbstractSubjectFilter::Allowed;
}

bool Qtilities::Core::SubjectTypeFilter::initializeDetachment(QObject* obj, bool subject_deleted) {
    Q_UNUSED(obj)
    Q_UNUSED(subject_deleted)

    return true;
}

void Qtilities::Core::SubjectTypeFilter::finalizeDetachment(QObject* obj, bool detachment_successful, bool subject_deleted) {
    Q_UNUSED(obj)
    Q_UNUSED(detachment_successful)
    Q_UNUSED(subject_deleted)
}

QStringList Qtilities::Core::SubjectTypeFilter::monitoredProperties() const {
    return QStringList();
}

QStringList Qtilities::Core::SubjectTypeFilter::reservedProperties() const {
    return QStringList();
}

bool Qtilities::Core::SubjectTypeFilter::handleMonitoredPropertyChange(QObject* obj, const char* property_name, QDynamicPropertyChangeEvent* propertyChangeEvent) {
    Q_UNUSED(obj)
    Q_UNUSED(property_name)
    Q_UNUSED(propertyChangeEvent)
    return false;
}

QString Qtilities::Core::SubjectTypeFilter::groupName() const {
    return d->known_objects_group_name;
}

void Qtilities::Core::SubjectTypeFilter::addSubjectType(SubjectTypeInfo subject_type_info) {
    if (!observer)
        d->known_subject_types.append(subject_type_info);
}

bool Qtilities::Core::SubjectTypeFilter::isKnownType(const QString& meta_type) const {
    for (int i = 0; i < d->known_subject_types.count(); i++) {
        if (d->known_subject_types.at(i).d_meta_type == meta_type) {
            if (!d->inversed_filtering)
                return true;
        } else {
            if (d->inversed_filtering)
                return true;
        }
    }

    return false;
}

QList<Qtilities::Core::SubjectTypeInfo> Qtilities::Core::SubjectTypeFilter::knownSubjectTypes() const {
    return d->known_subject_types;
}

void Qtilities::Core::SubjectTypeFilter::enableInverseFiltering(bool enabled) {
    if (!observer)
        d->inversed_filtering = enabled;
}

bool Qtilities::Core::SubjectTypeFilter::inverseFilteringEnabled() const {
    return d->inversed_filtering;
}

Qtilities::Core::Interfaces::IFactoryData Qtilities::Core::SubjectTypeFilter::factoryData() const {
    IFactoryData factoryData(FACTORY_SUBJECT_FILTERS,FACTORY_TAG_SUBJECT_TYPE_FILTER,objectName());
    return factoryData;
}

Qtilities::Core::Interfaces::IExportable::ExportModeFlags Qtilities::Core::SubjectTypeFilter::supportedFormats() const {
    IExportable::ExportModeFlags flags = 0;
    flags |= IExportable::Binary;
    return flags;
}

Qtilities::Core::Interfaces::IExportable::Result Qtilities::Core::SubjectTypeFilter::exportBinary(QDataStream& stream, QList<QVariant> params) const {
    Q_UNUSED(params)
    Q_UNUSED(stream)

    IFactoryData factory_data = factoryData();
    factory_data.exportBinary(stream);

    stream << d->inversed_filtering;
    stream << d->known_objects_group_name;
    stream << (quint32) d->known_subject_types.count();
    for (int i = 0; i < d->known_subject_types.count(); i++) {
        stream << d->known_subject_types.at(i).d_meta_type;
        stream << d->known_subject_types.at(i).d_name;
    }

    return IExportable::Complete;
}

Qtilities::Core::Interfaces::IExportable::Result Qtilities::Core::SubjectTypeFilter::importBinary(QDataStream& stream, QList<QPointer<QObject> >& import_list, QList<QVariant> params) {
    Q_UNUSED(import_list)
    Q_UNUSED(params)
    Q_UNUSED(stream)

    stream >> d->inversed_filtering;
    stream >> d->known_objects_group_name;
    quint32 known_type_count;
    stream >> known_type_count;
    int known_type_count_int = known_type_count;
    d->known_subject_types.clear();
    for (int i = 0; i < known_type_count_int; i++) {
        QString meta_type;
        QString name;
        stream >> meta_type;
        stream >> name;
        SubjectTypeInfo new_type(meta_type, name);
        d->known_subject_types << new_type;
    }

    return IExportable::Complete;
}

bool Qtilities::Core::SubjectTypeFilter::exportXML(QDomDocument* doc, QDomElement* object_node, QList<QVariant> params) const {
    Q_UNUSED(doc)
    Q_UNUSED(object_node)
    Q_UNUSED(params)

    return false;
}

bool Qtilities::Core::SubjectTypeFilter::importXML(QDomDocument* doc, QDomElement* object_node, QList<QVariant> params) {
    Q_UNUSED(doc)
    Q_UNUSED(object_node)
    Q_UNUSED(params)

    return false;
}
