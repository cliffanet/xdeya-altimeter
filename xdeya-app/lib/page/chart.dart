import 'package:flutter/material.dart';
import 'package:chart_sparkline/chart_sparkline.dart';
import '../data/trkdata.dart';
import 'dart:developer' as developer;

class PageChart extends StatelessWidget {
    const PageChart({ super.key });
    
    @override
    Widget build(BuildContext context) {
        return ValueListenableBuilder(
            valueListenable: trk.notifyData,
            builder: (BuildContext context, inf, Widget? child) {
                if (trk.data.isEmpty) {
                    return const Center(
                        child: Text('нет данных')
                    );
                }

                return Sparkline(
                    data: trk.alt,
                    kLine: ['first'],
                    // backgroundColor: Colors.red,
                    // lineColor: Colors.lightGreen[500]!,
                    // fillMode: FillMode.below,
                    // fillColor: Colors.lightGreen[200]!,
                    // pointsMode: PointsMode.none,
                    // pointSize: 5.0,
                    // pointColor: Colors.amber,
                    // useCubicSmoothing: true,
                    // lineWidth: 1.0,
                    // gridLinelabelPrefix: '\$',
                    gridLinelabel: (v) => '${v.ceil()}m',
                    gridLineLabelPrecision: 1,
                    enableGridLines: true,
                    //averageLine: true,
                    //averageLabel: true,
                    // kLine: ['max', 'min', 'first', 'last'],
                    max: trk.altMax.toDouble(),
                    min: 0,
                    // enableThreshold: true,
                    // thresholdSize: 0.1,
                    // lineGradient: LinearGradient(
                    //   begin: Alignment.topCenter,
                    //   end: Alignment.bottomCenter,
                    //   colors: [Colors.purple[800]!, Colors.purple[200]!],
                    // ),
                    // fillGradient: LinearGradient(
                    //   begin: Alignment.topCenter,
                    //   end: Alignment.bottomCenter,
                    //   colors: [Colors.red[800]!, Colors.red[200]!],
                    // ),
                );
            },
        );
    }
}