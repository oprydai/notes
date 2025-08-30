/****************************************************************************
** Meta object code from reading C++ file 'DatabaseManager.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/db/DatabaseManager.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DatabaseManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_DatabaseManager_t {
    QByteArrayData data[10];
    char stringdata0[115];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_DatabaseManager_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_DatabaseManager_t qt_meta_stringdata_DatabaseManager = {
    {
QT_MOC_LITERAL(0, 0, 15), // "DatabaseManager"
QT_MOC_LITERAL(1, 16, 9), // "noteSaved"
QT_MOC_LITERAL(2, 26, 0), // ""
QT_MOC_LITERAL(3, 27, 6), // "noteId"
QT_MOC_LITERAL(4, 34, 11), // "noteDeleted"
QT_MOC_LITERAL(5, 46, 11), // "folderSaved"
QT_MOC_LITERAL(6, 58, 8), // "folderId"
QT_MOC_LITERAL(7, 67, 13), // "folderDeleted"
QT_MOC_LITERAL(8, 81, 17), // "autoSaveTriggered"
QT_MOC_LITERAL(9, 99, 15) // "performAutoSave"

    },
    "DatabaseManager\0noteSaved\0\0noteId\0"
    "noteDeleted\0folderSaved\0folderId\0"
    "folderDeleted\0autoSaveTriggered\0"
    "performAutoSave"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DatabaseManager[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   44,    2, 0x06 /* Public */,
       4,    1,   47,    2, 0x06 /* Public */,
       5,    1,   50,    2, 0x06 /* Public */,
       7,    1,   53,    2, 0x06 /* Public */,
       8,    0,   56,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       9,    0,   57,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    6,
    QMetaType::Void, QMetaType::Int,    6,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void DatabaseManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DatabaseManager *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->noteSaved((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->noteDeleted((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->folderSaved((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->folderDeleted((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->autoSaveTriggered(); break;
        case 5: _t->performAutoSave(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (DatabaseManager::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseManager::noteSaved)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (DatabaseManager::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseManager::noteDeleted)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (DatabaseManager::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseManager::folderSaved)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (DatabaseManager::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseManager::folderDeleted)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (DatabaseManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseManager::autoSaveTriggered)) {
                *result = 4;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject DatabaseManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_DatabaseManager.data,
    qt_meta_data_DatabaseManager,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *DatabaseManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DatabaseManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_DatabaseManager.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int DatabaseManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void DatabaseManager::noteSaved(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void DatabaseManager::noteDeleted(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void DatabaseManager::folderSaved(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void DatabaseManager::folderDeleted(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void DatabaseManager::autoSaveTriggered()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
