
import 'package:vector_math/vector_math_64.dart';
import 'package:flutter/material.dart';
import 'dart:developer' as developer;

class ViewTransform {
    double rx = 0;
    double ry = 0;
    double rz = 0;
    double scale = 1.0;

    ViewTransform() { reset(); }

    void reset() {
        rx = 0;
        ry = 0;
        rz = 0;
        scale = 0.9;
    }
}

class ViewMatrix {
    final _notify = ValueNotifier<int>(0);
    get notify => _notify;

    // pos - позиция в треке. Храним тут, т.к. неважно, где,
    // а тут мы сможем _notify обновлять
    int _pos = 0;
    int get pos => _pos;
    set pos(int p) {
        if (_pos == p) return;
        _pos = p;
        _notify.value ++;
    }
    bool _posVisible = false;
    bool get posVisible => _posVisible;
    set posVisible(v) {
        if (_posVisible == v) return;
        _posVisible = v;
        _notify.value ++;
    }

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
    final _trns = ViewTransform(); // x, y, scale
    var _pnt = Offset.zero;

    void scaleStart(ScaleStartDetails details) {
        // Сохранение начальных значений
        _pnt = details.focalPoint;
    }
    void scaleUpdate(ScaleUpdateDetails details) {
        final delta = details.focalPoint - _pnt;
        _pnt = details.focalPoint;

        _trns.rx += delta.dx / 100.0;
        _trns.ry += delta.dy / 100.0;
        _trns.rz += details.rotation;
        _trns.scale *= details.scale;

        //developer.log('scaleUpdate: ${delta} $_trns');
        _updView();
        _notify.value ++;
    }

    // матрица преобразования
    Matrix4 _view = Matrix4.identity();
    Matrix4 get matr => _view;
    void _updView() {
        _view = Matrix4.identity();
        _view
            // смещаем центр координат в центр нашей фигуры
            ..translate(_size.width/2, _size.height/2)
            // делаем нужные преобразования
            ..rotateZ(_trns.rz)
            ..rotateX(-_trns.ry)
            ..rotateY(_trns.rx)
            ..scale(_trns.scale)
            // смещаем центр координат обратно
            ..translate(-_size.width/2, -_size.height/2);
    }
    void reset() {
        _trns.reset();
        _updView();
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
