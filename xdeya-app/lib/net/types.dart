import 'binproto.dart';
import '../data/dtime.dart';

const String pkLogItem = 'NnaaiiIIIiiINC nNNNNNN';
                         
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
    String get timeTakeoff => sec2time( (toff['tmoffset'] ?? 0) ~/ 1000 );
    String get dtBeg => dt2format(tm);
    int    get altBeg => beg['alt'] ?? 0;
    String get timeFF => sec2time(((cnp['tmoffset']??0) - (beg['tmoffset']??0)) ~/ 1000);
    int    get altCnp => cnp['alt'] ?? 0;
    String get timeCnp => sec2time(((end['tmoffset']??0) - (cnp['tmoffset']??0)) ~/ 1000);
}



class WiFiPass {
    final String ssid;
    final String pass;
    final String _orig_ssid;
    final String _orig_pass;

    WiFiPass({
        required this.ssid,
        required this.pass,
    }) : _orig_ssid = '', _orig_pass = '';

    WiFiPass.byvars(List<dynamic> vars) :
        ssid        = (vars.isNotEmpty) && (vars[0]) is String ? vars[0] : '',
        _orig_ssid  = (vars.isNotEmpty) && (vars[0]) is String ? vars[0] : '',
        pass        =  (vars.length > 1) && (vars[1]) is String ? vars[1] : '',
        _orig_pass  =  (vars.length > 1) && (vars[1]) is String ? vars[1] : '';
    
    WiFiPass.edited(this.ssid, this.pass, WiFiPass orig) :
        _orig_ssid  = orig._orig_ssid,
        _orig_pass  = orig._orig_pass;
    
    bool get isChanged {
        return
            (_orig_ssid != ssid) ||
            (_orig_pass != pass);
    }
    bool get isValid => ssid.isNotEmpty;
}

class FileItem {
    final String name;
    final String? path;
    final int size;
    bool checked = true;

    FileItem({
        required this.name,
        required this.size,
        this.path
    });

    FileItem.byvars(List<dynamic> vars) :
        size = (vars.isNotEmpty) && (vars[0]) is int ? vars[0] : '',
        name = (vars.length > 1) && (vars[1]) is String ? vars[1] : '',
        path = null;
}
