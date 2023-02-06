import 'package:flutter/material.dart';
import 'pager.dart';

void main() {
    runApp(const MaterialApp(
        title: 'Xde-Ya altimeter',
        home: Pager(page: PageCode.discovery),
    ));
}
