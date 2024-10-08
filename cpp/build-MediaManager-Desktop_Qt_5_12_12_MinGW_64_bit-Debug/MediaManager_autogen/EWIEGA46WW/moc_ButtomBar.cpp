/****************************************************************************
** Meta object code from reading C++ file 'ButtomBar.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.12)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../MediaManager/ButtomBar.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ButtomBar.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.12. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ButtomBar_t {
    QByteArrayData data[9];
    char stringdata0[132];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ButtomBar_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ButtomBar_t qt_meta_stringdata_ButtomBar = {
    {
QT_MOC_LITERAL(0, 0, 9), // "ButtomBar"
QT_MOC_LITERAL(1, 10, 13), // "slotPlayVideo"
QT_MOC_LITERAL(2, 24, 0), // ""
QT_MOC_LITERAL(3, 25, 11), // "slotAddFile"
QT_MOC_LITERAL(4, 37, 15), // "slotChangeSpeed"
QT_MOC_LITERAL(5, 53, 17), // "slotVolumeChanged"
QT_MOC_LITERAL(6, 71, 18), // "slotUpdateProgress"
QT_MOC_LITERAL(7, 90, 22), // "slotVideoDoubleClicked"
QT_MOC_LITERAL(8, 113, 18) // "slotSliderReleased"

    },
    "ButtomBar\0slotPlayVideo\0\0slotAddFile\0"
    "slotChangeSpeed\0slotVolumeChanged\0"
    "slotUpdateProgress\0slotVideoDoubleClicked\0"
    "slotSliderReleased"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ButtomBar[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   49,    2, 0x0a /* Public */,
       3,    0,   50,    2, 0x0a /* Public */,
       4,    0,   51,    2, 0x0a /* Public */,
       5,    0,   52,    2, 0x0a /* Public */,
       6,    0,   53,    2, 0x0a /* Public */,
       7,    0,   54,    2, 0x0a /* Public */,
       8,    0,   55,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Bool,
    QMetaType::Void,

       0        // eod
};

void ButtomBar::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ButtomBar *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->slotPlayVideo(); break;
        case 1: _t->slotAddFile(); break;
        case 2: _t->slotChangeSpeed(); break;
        case 3: _t->slotVolumeChanged(); break;
        case 4: _t->slotUpdateProgress(); break;
        case 5: { bool _r = _t->slotVideoDoubleClicked();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 6: _t->slotSliderReleased(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ButtomBar::staticMetaObject = { {
    &QWidget::staticMetaObject,
    qt_meta_stringdata_ButtomBar.data,
    qt_meta_data_ButtomBar,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ButtomBar::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ButtomBar::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ButtomBar.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ButtomBar::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
