import 'dart:typed_data';
import 'dart:ui' as ui;
import 'package:flutter/material.dart';
import 'package:flutter/semantics.dart';
import 'package:vector_math/vector_math_64.dart' hide Colors;
import 'dart:developer' as developer;

import 'viewmatrix.dart';
import '../data/trkdata.dart';
import '../data/trkview.dart';

class CubePainter extends CustomPainter {
    final ViewMatrix view;
    final ui.Image ? img;
    final void Function(Size size) ?onSize;
    CubePainter({ required this.view, this.onSize, this.img });

    @override
    void paint(Canvas canvas, Size size) {
        //ByteData data = img.toByteData();
        //if (img != null) {
        //    canvas.drawImage(img!, const Offset(100.0, 0.0), Paint());
       // }
        // Применение матрицы вида
        if (onSize != null) {
            onSize!(size);
        }
        view.size = size;

        if ((size.shortestSide <= 0) || (trk.area == null)) {
            return;
        }

        // Рисование 3D линии
        final tr = TrkPointTransform(trk.area!, size);
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

        final paint = Paint()
            ..color = Colors.black
            ..strokeWidth = 3;
        canvas.drawLine(
            view.getPoint(
                Vector3(
                    size.width / 2 - 50,
                    size.height / 2 - 50,
                    -50
                )
            ),
            view.getPoint(
                Vector3(
                    size.width / 2 + 50,
                    size.height / 2 - 50,
                    -50
                )
            ),
            paint,
        );
        canvas.drawLine(
            view.getPoint(
                Vector3(
                    size.width / 2 + 50,
                    size.height / 2 - 50,
                    -50
                )
            ),
            view.getPoint(
                Vector3(
                    size.width / 2 + 50,
                    size.height / 2 + 50,
                    -50
                )
            ),
            paint,
        );
        canvas.drawLine(
            view.getPoint(
                Vector3(
                    size.width / 2 + 50,
                    size.height / 2 + 50,
                    -50
                )
            ),
            view.getPoint(
                Vector3(
                    size.width / 2 - 50,
                    size.height / 2 + 50,
                    -50
                )
            ),
            paint,
        );
    }

    @override
    bool shouldRepaint(CubePainter oldDelegate) => true;
}

/*
class ImagePainter extends CustomPainter {
  List<ui.Image> images = new List<ui.Image>();
  ImagePainter(
      {Key key,
      @required this.noOfSlice,
      @required this.images,
      @required this.rotation,
      this.boxfit = BoxFit.contain})
      :
        // : path = new Path()
        //     ..addOval(new Rect.fromCircle(
        //       center: new Offset(75.0, 75.0),
        //       radius: 40.0,
        //     )),
        tickPaint = new Paint() {
    tickPaint.strokeWidth = 2.5;
  }
  final int noOfSlice;
  final tickPaint;
  final BoxFit boxfit;
  ui.ImageByteFormat img;
  ui.Rect rect, inputSubrect, outputSubrect;
  Size imageSize;
  FittedSizes sizes;
  double radius,
      rotation = 0.0,
      _x,
      _y,
      _angle,
      _radiun,
      _baseLength,
      _imageCircleradius,
      _incircleRadius,
      _imageOffset = 0.0,
      _imageSizeConst = 0.0;

  @override
  void paint(ui.Canvas canvas, ui.Size size) {
    print("image data:: $images");
    radius = size.width / 2;
    _angle = 360 / (noOfSlice * 2.0);
    _radiun = (_angle * pi) / 180;
    _baseLength = 2 * radius * sin(_radiun);
    _incircleRadius = (_baseLength / 2) * tan(_radiun);
    if (noOfSlice == 4) {
      _imageOffset = 30.0;
      _imageSizeConst = 30.0;
      _x = 8.60;
      _y = 4.10;
    } else if (noOfSlice == 6) {
      _imageOffset = 20.0;
      _x = 10.60;
      _y = 5.60;
    } else if (noOfSlice == 8) {
      _imageOffset = 40.0;
      _imageSizeConst = 30.0;
      _x = 12.90;
      _y = 6.60;
    }

    //print("circle radisu:: $_incircleRadius");

    canvas.save();
    canvas.translate(size.width / 2, size.height / 2);
    canvas.rotate(-rotation);
    int incr = 0;
    rect = ui.Offset((size.width / _x), size.width / _y) & new Size(0.0, 0.0);

    imageSize = new Size(size.width * 1.5, size.width * 1.5);
    sizes = applyBoxFit(
        boxfit,
        imageSize,
        new Size(size.width / 2 * .50 + _incircleRadius * .8,
            size.width / 2 * .50 + _incircleRadius * .8));
    inputSubrect =
        Alignment.center.inscribe(sizes.source, Offset.zero & imageSize);
    outputSubrect = Alignment.center.inscribe(sizes.destination, rect);
    if (images.length == noOfSlice && images.isNotEmpty)
      for (var i = 1; i <= noOfSlice * 2; ++i) {
        if (i % 2 != 0) {
          canvas.drawLine(
            new Offset(0.0, 0.0),
            new Offset(0.0, size.width / 2 - 4.2),
            tickPaint,
          );
        } else {
          canvas.save();
          canvas.translate(-0.0, -((size.width) / 2.2));
          ui.Image image = images[incr];
          if (image != null) {
            canvas.drawImageRect(
                image, inputSubrect, outputSubrect, new Paint());
          }

          canvas.restore();
          incr++;
        }
        canvas.rotate(2 * pi / (noOfSlice * 2.0));
      }
    canvas.restore();
  }

  @override
  bool shouldRepaint(CustomPainter oldDelegate) {
    return false;
  }
}
*/
