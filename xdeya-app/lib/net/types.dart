import 'binproto.dart';

const String pkLogItem = 'NnaaiiNNNiiNNC nNNNNNN';
const List<String> fldLogItem = [
    'tmoffset',     // время от начала измерений        (ms)
    'flags',        // флаги: валидность
    'state',        // статус высотомера (земля, подъём, падение, под куполом)
    'direct',       // направление движения по высоте
    'alt',          // высота по барометру              (m)
    'altspeed',     // скорость падения по барометру    (cm/s)
    'lon',          // Longitude                        (deg * 10^7)
    'lat',          // Latitude                         (deg * 10^7)
    'hspeed',       // Ground speed                     (cm/s)
    'heading',      // направление движения             (deg)
    'gpsalt',       // высота по NAV (над ур моря)      (m)
    'vspeed',       // 3D speed                         (cm/s)
    'gpsdage',      // gps data age  // тут вместо millis уже используется только 1 байт для хранения, можно пересмотреть формат
    'sat',          // количество найденных спутников
    
    'batval',       // raw-показания напряжения на батарее
    // поля для отладки
    'hAcc',         // Horizontal accuracy estimate (mm)
    'vAcc',         // Vertical accuracy estimate   (mm)
    'sAcc',         // Speed accuracy estimate      (cm/s)
    'cAcc',         // Heading accuracy estimate    (deg * 10^5)
    'millis',       // для отладки времени tmoffset
    'msave',
];

    
String _sec2time(int sec) {
    int min = sec ~/ 60;
    sec -= min*60;
    int hour = min ~/ 60;
    min -= hour*60;

    return '$hour:${min.toString().padLeft(2,'0')}:${sec.toString().padLeft(2,'0')}';
}
String _dt2format(DateTime dt) {
    return
        '${dt.day}.${dt.month.toString().padLeft(2,'0')}.${dt.year} '
        '${dt.hour.toString().padLeft(2, ' ')}:${dt.minute.toString().padLeft(2,'0')}';
}

class LogBook {
    static const pk = 'NNT$pkLogItem$pkLogItem$pkLogItem$pkLogItem';

    final int num;
    final int key;
    final DateTime tm;
    final Struct toff;
    final Struct beg;
    final Struct cnp;
    final Struct end;

    LogBook({
        required this.num,
        required this.key,
        required this.tm,
        required this.toff,
        required this.beg,
        required this.cnp,
        required this.end
    });

    LogBook.byvars(List<dynamic> vars) :
        this(
            num: (vars.isNotEmpty) && (vars[0]) is int ? vars[0] : 0,
            key: (vars.length > 1) && (vars[1]) is int ? vars[1] : 0,
            tm:  (vars.length > 2) && (vars[2]) is DateTime ? vars[2] : DateTime(0),
            toff: fldUnpack(fldLogItem, vars, 3),
            beg:  fldUnpack(fldLogItem, vars, 3 + fldLogItem.length),
            cnp:  fldUnpack(fldLogItem, vars, 3 + fldLogItem.length * 2),
            end:  fldUnpack(fldLogItem, vars, 3 + fldLogItem.length * 3)
        );
    
    String get date => '${tm.day}.${tm.month.toString().padLeft(2,'0')}.${tm.year}';
    String get timeTakeoff => _sec2time( (toff['tmoffset'] ?? 0) ~/ 1000 );
    String get dtBeg => _dt2format(tm);
    int    get altBeg => beg['alt'] ?? 0;
    String get timeFF => _sec2time(((cnp['tmoffset']??0) - (beg['tmoffset']??0)) ~/ 1000);
    int    get altCnp => cnp['alt'] ?? 0;
    String get timeCnp => _sec2time(((end['tmoffset']??0) - (cnp['tmoffset']??0)) ~/ 1000);
}

class TrkItem {
    final int id;
    final int flags;
    final int jmpnum;
    final int jmpkey;
    final DateTime tmbeg;
    final int fsize;
    final int fnum;

    TrkItem({
        required this.id,
        required this.flags,
        required this.jmpnum,
        required this.jmpkey,
        required this.tmbeg,
        required this.fsize,
        required this.fnum,
    });

    TrkItem.byvars(List<dynamic> vars) :
        this(
            id:     (vars.isNotEmpty) && (vars[0]) is int ? vars[0] : 0,
            flags:  (vars.length > 1) && (vars[1]) is int ? vars[1] : 0,
            jmpnum: (vars.length > 2) && (vars[2]) is int ? vars[2] : 0,
            jmpkey: (vars.length > 3) && (vars[3]) is int ? vars[3] : 0,
            tmbeg:  (vars.length > 4) && (vars[4]) is DateTime ? vars[4] : DateTime(0),
            fsize:  (vars.length > 5) && (vars[5]) is int ? vars[5] : 0,
            fnum:   (vars.length > 6) && (vars[6]) is int ? vars[6] : 0,
        );

    String get dtBeg => _dt2format(tmbeg);
}



class TrkInfo {
    final int id;
    final int flags;
    final int jmpnum;
    final int jmpkey;
    final DateTime tmbeg;
    final int fsize;
    final int chksum;

    TrkInfo({
        required this.id,
        required this.flags,
        required this.jmpnum,
        required this.jmpkey,
        required this.tmbeg,
        required this.fsize,
        required this.chksum,
    });

    TrkInfo.byvars(List<dynamic> vars) :
        this(
            id:     (vars.isNotEmpty) && (vars[0]) is int ? vars[0] : 0,
            flags:  (vars.length > 1) && (vars[1]) is int ? vars[1] : 0,
            jmpnum: (vars.length > 2) && (vars[2]) is int ? vars[2] : 0,
            jmpkey: (vars.length > 3) && (vars[3]) is int ? vars[3] : 0,
            tmbeg:  (vars.length > 4) && (vars[4]) is DateTime ? vars[4] : DateTime(0),
            fsize:  (vars.length > 5) && (vars[5]) is int ? vars[5] : 0,
            chksum: (vars.length > 6) && (vars[6]) is int ? vars[6] : 0,
        );

    String get dtBeg => _dt2format(tmbeg);
}

class WiFiPass {
    final String ssid;
    final String pass;

    WiFiPass({
        required this.ssid,
        required this.pass,
    });

    WiFiPass.byvars(List<dynamic> vars) :
        this(
            ssid:   (vars.isNotEmpty) && (vars[0]) is String ? vars[0] : '',
            pass:   (vars.length > 1) && (vars[1]) is String ? vars[1] : '',
        );
}
