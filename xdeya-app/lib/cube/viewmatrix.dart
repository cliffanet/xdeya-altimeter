import 'dart:math';
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
    final _trbeg = Matrix4.identity();
    final _trend = Matrix4.identity();
    set size(Size sz) {
        _size = sz;
        _trbeg
            ..setIdentity()
            ..translate(_size.width/2, _size.height/2);
        _trend
            ..setIdentity()
            ..translate(-_size.width/2, -_size.height/2);
        // тут не нужно оповещать о необходимости перерисовать,
        // т.к. изменение size мы делаем уже из метода перерисовки
        //_notify.value ++;
    }

    // матрица преобразования
    final Matrix4 _view = Matrix4.identity();
    Matrix4 get matr => _view;

    // вращение
    var _pnt = Offset.zero;
    final _rota = Matrix4.identity();
    final _rbeg = Matrix4.identity();

    // Величина вращения вокруг оси
    // На входе:
    //  - axis      - ось в базовом положении любой длины
    //  - rotate    - вектор вращения, который задан из scaleUpdate
    double axisRotate(Vector3 axis, Vector2 rotate) {
        // Вращаем по _rbeg
        final v = _rbeg.transform3(Vector3.copy(axis));
        final v2 = Vector2(v.x, v.y);
        // угол к оси OY
        final ang = -v2.angleToSigned(Vector2(0, 1));
        //developer.log('axisRotate: $axis => $v => $ang / ${ v2.length }');
        //if ((v.x == 0) && (v.y == 0)) return 0;
        // на этот угол надо провращать вектор rotate
        // Координата X - это и будет необходимое усилие вращения
        final r = rotate.x * cos(ang) - rotate.y * sin(ang);
        // однако, это усилие надо помножить на длину вектора v
        return r * v2.length;
    }
    Vector3 axisRotate3(Vector2 rotate) {
        final rx = axisRotate(Vector3(1, 0, 0), rotate);
        final ry = axisRotate(Vector3(0, 1, 0), rotate);
        final rz = axisRotate(Vector3(0, 0, 1), rotate);
        return Vector3(rx, ry, rz);
    }

    // scale
    final _scal = Matrix4.identity();
    final _sbeg = Matrix4.identity();
    void scaleStart(ScaleStartDetails details) {
        // Сохранение начальных значений
        _pnt = details.focalPoint;

        _rbeg.setFrom(_rota);
        _sbeg.setFrom(_scal);
    }
    void scaleUpdate(ScaleUpdateDetails details) {
        final delta = details.focalPoint - _pnt;
        
        final r = Vector2(delta.dx / 100.0, -1*(delta.dy / 100.0));
        final rr = axisRotate3(r);
        //developer.log('rotate: $rr');

        _rota.setFrom(
            Matrix4.copy(_rbeg)
            ..rotateX(rr.x)
            ..rotateY(rr.y)
            ..rotateZ(rr.z)
        );
        _scal.setFrom(
            Matrix4.copy(_sbeg)
            ..scale(details.scale)
        );

        _view.setFrom(
            _trbeg * _rota * _scal * _trend
        );

        _notify.value ++;
    }
    void scaleChange(double scroll) {
        final scale = 1 / (scroll > 0 ? (100 + scroll) / 100 : -3.5 / scroll);
        developer.log('scale: $scale ($scroll)');
        _scal.scale(scale);

        _view.setFrom(
            _trbeg * _rota * _scal * _trend
        );

        _notify.value ++;
    }
    
    void reset() {
        _rota.setIdentity();
        _scal.setIdentity();
        _view.setIdentity();
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
