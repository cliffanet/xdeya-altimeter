import 'dart:typed_data';
import 'dart:ui' as ui;
import 'package:flutter/material.dart';
import 'package:flutter/semantics.dart';
import 'package:vector_math/vector_math_64.dart' hide Colors;
import 'dart:developer' as developer;

import 'viewmatrix.dart';
import '../data/trkdata.dart';
import '../data/trkview.dart';
import '../data/trkimg.dart';

class CubePainter extends CustomPainter {
    final ViewMatrix view;
    final void Function(Size size) ?onSize;
    CubePainter({ required this.view, this.onSize });

    List<ui.Vertices> imgVertices(TrkImage img, TrkPointTransform tr) {
        if (!img.imgLoaded) return [];

        {
            final lon = img.lon(0);
            final lat = img.lat(0);
            final x1 = tr.mapLon(lon);
            final y1 = tr.mapLat(lat);
            final o = Offset(x1, y1);
        }
        {
            final lon = img.lon(img.width);
            final lat = img.lat(img.height);
            final x1 = tr.mapLon(lon);
            final y1 = tr.mapLat(lat);
            final o = Offset(x1, y1);
        }

        final lst = <ui.Vertices>[];
        for (var v in img.vertex!) {
            int n=0;
            final vertices = Float32List(v.height * img.width * 2);
            for (int y = 0; y < v.height; y++) {
                for (int x = 0; x < img.width; x++) {
                    final x1    = tr.mapLon( img.lon(x) );
                    final y1    = tr.mapLat( img.lat(y+v.ybeg) );
                    final p     = view.getPoint(Vector3(x1, y1, tr.begz*tr.mapz));
                    vertices[n]     = p.dx;
                    vertices[n+1]   = p.dy;
                    n += 2;
                }
            }

            lst.add(
                ui.Vertices.raw(
                    VertexMode.triangles,
                    vertices,
                    colors:     v.colors,
                    indices:    v.indices
                )
            );
        }
        return lst;
    }

    @override
    void paint(Canvas canvas, Size size) {
        if (onSize != null) {
            onSize!(size);
        }
        view.size = size;

        if ((size.shortestSide <= 0) || (trk.area == null)) {
            return;
        }

        canvas.save();
        
        // коэфициенты преобразования
        final tr = TrkPointTransform(trk.area!, size);
        
        if ((trk.img != null) && trk.img!.imgLoaded) {
            for (final v in imgVertices(trk.img!, tr)) {
                canvas.drawVertices(v, BlendMode.src, Paint());
            }
        }

        // Рисование 3D линии
        final data = trk.seg.where((s) => s.satValid).map((s) => TrkViewSeg.bySeg(s, tr)).toList();
        for (var seg in data) {
            if (seg.pnt.isEmpty) {
                continue;
            }
            final paint = Paint()
                ..color = seg.color
                ..strokeWidth = 2;
            
            var p1 = view.getPoint(seg.pnt[0].pnt);
            for (var pnt in seg.pnt) {
                final p2 = view.getPoint(pnt.pnt);
                canvas.drawLine(p1, p2, paint);
                p1 = p2;
            }
        }

        // Рисование точки с инфой
        if (view.posVisible &&
            (view.pos > 0) && (view.pos < trk.data.length) &&
            trk.data[view.pos].satValid) {
            final ti = trk.data[view.pos];

            final p = view.getPoint(TrkViewPoint.byLogItem(ti, tr).pnt);
            final paintPoint = Paint()
                    ..color = //Colors.orange//
                                trkColor(ti['state'])
                    ..maskFilter = MaskFilter.blur(BlurStyle.normal, convertRadiusToSigma(2));
            canvas.drawCircle(p, 7, paintPoint);

            // info
            final text = TextPainter(
                text: TextSpan(
                    text: '${ti.time}\nверт: ${ti.dscrVert}\nгор: ${ti.dscrHorz}\n${ti.dscrKach}',
                    style: const TextStyle(
                        color: Colors.black,
                        fontSize: 10,
                    )
                ),
                textDirection: TextDirection.ltr,
            );
            text.layout();
            
            // rect
            final rect = RRect.fromRectXY(
                Rect.fromPoints(p+Offset(15, 15), p+Offset(25+text.width, 25+text.height)),
                7, 7
            );
            final paintInfo = Paint()
                    ..color = Colors.white.withAlpha(100)
                    ..maskFilter = MaskFilter.blur(BlurStyle.normal, convertRadiusToSigma(2));
            canvas.drawRRect(rect, paintInfo);

            // paint info
            text.paint(canvas, p+Offset(20, 20));
        }

        canvas.restore();
    }

    @override
    bool shouldRepaint(CubePainter oldDelegate) => true;

    static double convertRadiusToSigma(double radius) {
        return radius * 0.57735 + 0.5;
    }
}
