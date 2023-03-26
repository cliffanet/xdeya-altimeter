
import 'package:vector_math/vector_math_64.dart';
import 'package:flutter/material.dart';
import 'dart:developer' as developer;

class ViewMatrix {
    final _notify = ValueNotifier<int>(0);
    get notify => _notify;

    // размер viewport
    var _size = Size.zero;
    Size get size => _size;
    set size(Size sz) {
        _size = sz;
        _updView();
        // тут не нужно оповещать о необходимости перерисовать,
        // т.к. изменение size мы делаем уже из метода перерисовки
        //_notify.value ++;
    }

    // вращение
    final _trns = Vector3(0.0, 0.0, .9); // x, y, scale
    var _pnt = Offset.zero;

    void scaleStart(ScaleStartDetails details) {
        // Сохранение начальных значений
        _pnt = details.focalPoint;
    }
    void scaleUpdate(ScaleUpdateDetails details) {
        final delta = details.focalPoint - _pnt;
        _pnt = details.focalPoint;

        _trns.x += delta.dx / 100.0;
        _trns.y += delta.dy / 100.0;
        _trns.z *= details.scale;

        //developer.log('scaleUpdate: ${delta} $_trns');
        _updView();
        _notify.value ++;
    }

    // матрица преобразования
    Matrix4 _view = Matrix4.identity();
    void _updView() {
        _view = Matrix4.identity();
        _view
            // смещаем центр координат в центр нашей фигуры
            ..translate(_size.width/2, _size.height/2)
            // делаем нужные преобразования
            ..rotateX(-_trns.y)
            ..rotateY(_trns.x)
            ..scale(_trns.z)
            // смещаем центр координат обратно
            ..translate(-_size.width/2, -_size.height/2);
    }

    Offset getPoint(Vector3 pnt) {
        // метод .transform3 модифицирует Vector3, который мы ему передаём,
        // поэтому надо передавать копию.
        final pntTrans = _view.transform3( Vector3.copy(pnt) );

        return Offset(
            pntTrans.x,
            pntTrans.y
        );
    }
}
