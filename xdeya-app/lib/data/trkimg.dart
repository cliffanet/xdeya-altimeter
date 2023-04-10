import 'dart:typed_data';
import 'dart:ui' as ui;
import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;

import 'trktypes.dart';

class TrkImgVertice {
    final int height, ybeg;
    final Int32List colors;
    final Uint16List indices;
    TrkImgVertice(this.height, this.ybeg, this.colors, this.indices);
}
class TrkImage {
    static const int qwidth = 650;
    static const int qheight = 450;
    final double areaLonMin, areaLonMax, areaLonKoef, areaLatMin, areaLatMax;
    final int z;

    static double z2dx(int z, double koef) => z2dy(z) / koef * qwidth / qheight;
    static double z2dy(int z) => 360 / (1 << z);
    static int calcz(TrkArea area) {
        for (int z = 16; z > 0; z--) {
            final latMax = area.latCenter + z2dy(z)/2;
            if (latMax < area.latMax) continue;
            final lonMax = area.lonCenter + z2dx(z, area.lonKoef)/2;
            if (lonMax < area.lonMax) continue;
            return z-1;
        }
        return 0;
    }

    TrkImage(TrkArea area) :
        areaLonMin  = area.lonMin,
        areaLonMax  = area.lonMax,
        areaLonKoef = area.lonKoef,
        areaLatMin  = area.latMin,
        areaLatMax  = area.latMax,
        z           = calcz(area);
    
    double get dx       => z2dx(z, areaLonKoef);
    double get lonCenter=> (areaLonMax - areaLonMin) / 2 + areaLonMin;
    double get lonMin   => lonCenter - dx/2;
    double get lonMax   => lonCenter + dx/2;
    double lon(int x)   => isValid ? x * dx / width + lonMin : 0;
    double get dy       => z2dy(z);
    double get latCenter=> (areaLatMax - areaLatMin) / 2 + areaLatMin;
    double get latMin   => latCenter - dy/2;
    double get latMax   => latCenter + dy/2;
    double lat(int y)   => isValid ? (height-y) * dy / height + latMin : 0;

    // лучше всего запрос строить через "масштаб" z, т.к. в этом случае
    // мы точно знаем, какой размер в градусах у меньшей стороны: 360 делим на 2 в степени z-1.
    // z=0 - это мы не делим глобус, отрисовываем все 360 градусов (и ширину и высоту)
    // z=1 - делим глобус на 4 картинки (2 по горизонтали и 2 по вертикали)
    // z=2 - делим ещё на 2 по каждой оси.
    // Подробности: https://yandex.ru/dev/maps/jsapi/doc/2.1/theory/index.html
    // А если мы будем задавать диапазон координат, то яндекс всё равно выбирает масштаб,
    // в который эти координаты влезут, и углы картинки никогда не будут совпадать в этими координатами,
    // эти координаты будут всегда внутри картинки.
    // Лучше сами подберём нужный z, и зная его, центр и размеры картинки,
    // мы всегда определяем точно координаты углов этой картинки.
    String get query    =>  'l=sat,skl&'
                            'll=${lonCenter.toStringAsFixed(7)},${latCenter.toStringAsFixed(7)}&z=15&'
                            'size=$qwidth,$qheight';
    Map<String, dynamic> get querymap => 
        {
            'l':    'sat,skl',
            'll':   '${lonCenter.toStringAsFixed(7)},${latCenter.toStringAsFixed(7)}',
            'z':    z.toString(),
            'size': '$qwidth,$qheight'
        };
    
    // mapImg
    ui.Image ?_img;
    ui.Image ? get img => _img;
    bool get isValid => _img != null;
    bool operator () => _img != null;
    int get width   => isValid ? _img!.width   : 0;
    int get height  => isValid ? _img!.height  : 0;
    static const seqheight = 100;

    List<TrkImgVertice> ? _vertex;
    List<TrkImgVertice> ? get vertex => _vertex;
    bool get imgLoaded => (_img != null) && (_vertex != null);
    Future<void> loadImg() async {
        _img = null;
        _vertex = null;

        final url = Uri.https('static-maps.yandex.ru', '/1.x/', querymap);
        final res = await http.get(url);
        if (
                (res.statusCode != 200) || 
                !res.headers.containsKey('content-type') ||
                (res.headers['content-type'] != 'image/jpeg')
            ) {
            return;
        }
        _img = await decodeImageFromList(res.bodyBytes);
        // Получение ByteData объекта изображения
        final data = (await _img!.toByteData())!.buffer.asUint8List();

        // собираем массив цветов и массив индексов,
        // но разбиваем на подсписки, т.к. есть предельный размер блока,
        // который нормально прорисовывается
        final vert = <TrkImgVertice>[];
        var height1 = height;
        var ybeg = 0;
        while (height1 > 0) {
            int h = height1;
            if (h > seqheight) {
                h = seqheight;
                // каждый блок должен быть на одну строку шире,
                // чтобы блоки связывались между собой
                height1 -= seqheight-1;
            }
            else {
                height1 = 0;
            }
            vert.add(
                TrkImgVertice(
                    h, ybeg,
                    Int32List(width*h),
                    Uint16List((width-1)*(h-1)*6)
                )
            );
            ybeg += h-1;
        }
        
        for (var v in vert) {
            int n = 0;
            int d = 0;
            int c = v.ybeg*width*4;

            for (int y = 0; y < v.height; y++) {
                for (int x = 0; x < width; x++, n++, c+=4) {
                    v.colors[n] = (
                        ((data[c+3] & 0xff) << 24) |
                        ((data[c]   & 0xff) << 16) |
                        ((data[c+1] & 0xff) << 8)  |
                        ((data[c+2] & 0xff) << 0)
                    ) & 0xFFFFFFFF;
                    
                    if ((y<v.height-1) && (x<width-1)) {
                        final i1 = n;
                        final i2 = n+1;
                        final i3 = n+width;
                        final i4 = i3+1;
                        v.indices[d]  = i1;
                        v.indices[d+1]= i2;
                        v.indices[d+2]= i4;
                        v.indices[d+3]= i1;
                        v.indices[d+4]= i3;
                        v.indices[d+5]= i4;
                        d+=6;
                    }
                }
            }
        }
        _vertex = vert;
    }
}
