/****************************************************************************
** Meta object code from reading C++ file 'MarkdownEditor.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/ui/MarkdownEditor.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MarkdownEditor.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MarkdownEditor_t {
    QByteArrayData data[8];
    char stringdata0[109];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MarkdownEditor_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MarkdownEditor_t qt_meta_stringdata_MarkdownEditor = {
    {
QT_MOC_LITERAL(0, 0, 14), // "MarkdownEditor"
QT_MOC_LITERAL(1, 15, 14), // "contentChanged"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 17), // "autoSaveRequested"
QT_MOC_LITERAL(4, 49, 13), // "onTextChanged"
QT_MOC_LITERAL(5, 63, 17), // "onAutoSaveTimeout"
QT_MOC_LITERAL(6, 81, 16), // "insertCompletion"
QT_MOC_LITERAL(7, 98, 10) // "completion"

    },
    "MarkdownEditor\0contentChanged\0\0"
    "autoSaveRequested\0onTextChanged\0"
    "onAutoSaveTimeout\0insertCompletion\0"
    "completion"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MarkdownEditor[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   39,    2, 0x06 /* Public */,
       3,    0,   40,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    0,   41,    2, 0x08 /* Private */,
       5,    0,   42,    2, 0x08 /* Private */,
       6,    1,   43,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    7,

       0        // eod
};

void MarkdownEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MarkdownEditor *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->contentChanged(); break;
        case 1: _t->autoSaveRequested(); break;
        case 2: _t->onTextChanged(); break;
        case 3: _t->onAutoSaveTimeout(); break;
        case 4: _t->insertCompletion((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MarkdownEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MarkdownEditor::contentChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MarkdownEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MarkdownEditor::autoSaveRequested)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MarkdownEditor::staticMetaObject = { {
    QMetaObject::SuperData::link<QTextEdit::staticMetaObject>(),
    qt_meta_stringdata_MarkdownEditor.data,
    qt_meta_data_MarkdownEditor,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MarkdownEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MarkdownEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MarkdownEditor.stringdata0))
        return static_cast<void*>(this);
    return QTextEdit::qt_metacast(_clname);
}

int MarkdownEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTextEdit::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void MarkdownEditor::contentChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void MarkdownEditor::autoSaveRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
