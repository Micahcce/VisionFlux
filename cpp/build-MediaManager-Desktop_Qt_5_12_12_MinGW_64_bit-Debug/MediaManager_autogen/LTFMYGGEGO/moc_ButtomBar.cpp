/****************************************************************************
** Meta object code from reading C++ file 'ButtomBar.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.12)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../MediaManager/QtGUI/ButtomBar.h"
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
    QByteArrayData data[12];
    char stringdata0[172];
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
QT_MOC_LITERAL(4, 37, 12), // "debugAddFile"
QT_MOC_LITERAL(5, 50, 8), // "filePath"
QT_MOC_LITERAL(6, 59, 15), // "slotChangeSpeed"
QT_MOC_LITERAL(7, 75, 17), // "slotVolumeChanged"
QT_MOC_LITERAL(8, 93, 18), // "slotUpdateProgress"
QT_MOC_LITERAL(9, 112, 22), // "slotVideoDoubleClicked"
QT_MOC_LITERAL(10, 135, 17), // "slotSliderPressed"
QT_MOC_LITERAL(11, 153, 18) // "slotSliderReleased"

    },
    "ButtomBar\0slotPlayVideo\0\0slotAddFile\0"
    "debugAddFile\0filePath\0slotChangeSpeed\0"
    "slotVolumeChanged\0slotUpdateProgress\0"
    "slotVideoDoubleClicked\0slotSliderPressed\0"
    "slotSliderReleased"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ButtomBar[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   59,    2, 0x0a /* Public */,
       3,    0,   60,    2, 0x0a /* Public */,
       4,    1,   61,    2, 0x0a /* Public */,
       6,    0,   64,    2, 0x0a /* Public */,
       7,    0,   65,    2, 0x0a /* Public */,
       8,    0,   66,    2, 0x0a /* Public */,
       9,    0,   67,    2, 0x0a /* Public */,
      10,    0,   68,    2, 0x0a /* Public */,
      11,    0,   69,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Bool,
    QMetaType::Void,
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
        case 2: _t->debugAddFile((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 3: _t->slotChangeSpeed(); break;
        case 4: _t->slotVolumeChanged(); break;
        case 5: _t->slotUpdateProgress(); break;
        case 6: { bool _r = _t->slotVideoDoubleClicked();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 7: _t->slotSliderPressed(); break;
        case 8: _t->slotSliderReleased(); break;
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
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 9;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
