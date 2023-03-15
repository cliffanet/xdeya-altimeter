import '../net/binproto.dart';

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


class LogItem {
    final Struct _log;

    LogItem.byvars(List<dynamic> vars) :
        _log = fldUnpack(fldLogItem, vars);

    dynamic operator [](String f) => _log[f];

    bool get satValid   => ((_log['flags'] ?? 0) & 0x0001) > 0;
    double get lat      => _log['lat'] / 10000000;
    double get lon      => _log['lon'] / 10000000;
    String get crd      => '[$lat,$lon]';
    double get altspeed => _log['altspeed']/100;
    double get hspeed   => _log['hspeed']/100;

    String get time {
        int sec = (_log['tmoffset'] ?? 0) ~/ 1000;
        int min = sec ~/ 60;
        sec -= min*60;
        return '$min:${sec.toString().padLeft(2,'0')}';
    }
    
    String get dscrVert => '${_log['alt']} m (${ (altspeed).toStringAsFixed(1) } m/s)';
    String get dscrHorz => '${_log['heading']}гр (${ (hspeed).toStringAsFixed(1) } m/s)';
    String get dscrKach =>
                _log['altspeed'] < -10 ?
                    ' [кач: ${ (-1.0 * _log['hspeed'] / _log['altspeed']).toStringAsFixed(1) }]' :
                    '';
}
