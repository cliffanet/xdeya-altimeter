
String sec2time(int sec) {
    int min = sec ~/ 60;
    sec -= min*60;
    int hour = min ~/ 60;
    min -= hour*60;

    return '$hour:${min.toString().padLeft(2,'0')}:${sec.toString().padLeft(2,'0')}';
}
String dt2format(DateTime dt) {
    return
        '${dt.day}.${dt.month.toString().padLeft(2,'0')}.${dt.year} '
        '${dt.hour.toString().padLeft(2, ' ')}:${dt.minute.toString().padLeft(2,'0')}';
}
