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
