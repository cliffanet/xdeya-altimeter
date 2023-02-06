import 'dart:io';
import 'dart:typed_data';

enum BinProtoRecv { waitcmd, data, complete, clear }

class BinProto {
    static const hdrsz = 4;
    final int mgcsnd;
    final int mgcrcv;
    final List<int> _rcvBuf = [];
    BinProtoRecv _rsvstate = BinProtoRecv.waitcmd;
    int _rcvcmd = 0;
    int _rcvsz = 0;

    BinProtoRecv get rcvState => _rsvstate;
    int get rcvCmd => _rcvcmd;

    BinProto([String mgcSnd = '#', String mgcRcv = '#']) :
        mgcsnd = mgcSnd.codeUnitAt(0),
        mgcrcv = mgcRcv.codeUnitAt(0);

    void rcvClear() {
        _rsvstate = BinProtoRecv.waitcmd;
        _rcvcmd = 0;
        _rcvsz = 0;
    }

    bool rcvProcess([Uint8List ?data]) {
        if (data != null) {
            _rcvBuf.addAll(data);
        }

        if (_rsvstate == BinProtoRecv.clear) {
            // очистка текущей команды
            int sz = _rcvsz;
            if (sz > _rcvBuf.length) {
                sz = _rcvBuf.length;
            }
            if (sz > 0) {
                _rcvBuf.removeRange(0, sz);
                _rcvsz -= sz;
            }
            if (_rcvsz > 0) {
                return true;
            }

            // Стравили всё, что нужно, теперь ждём следующую команду
            rcvClear();
        }

        if (_rsvstate == BinProtoRecv.waitcmd) {
            if (_rcvBuf.length < hdrsz) {
                return true;
            }

            // hdrunpack
            if ((_rcvBuf[0] != mgcrcv) || (_rcvBuf[1] == 0)) {
                return false;
            }
            _rcvcmd = _rcvBuf[1];
            ByteData bsz = ByteData(2);
            bsz.buffer.asUint8List().setAll(0, _rcvBuf.getRange(2, 4));
            _rcvsz = bsz.getUint16(0, Endian.big);
            _rcvBuf.removeRange(0, 4);


            _rsvstate = BinProtoRecv.data;
        }

        if (_rsvstate == BinProtoRecv.data) {
            if (_rcvBuf.length < _rcvsz) {
                return true;
            }

            // Мы получили нужный объём данных
            _rsvstate = BinProtoRecv.complete;
        }

        return true;
    }

    List<dynamic>? rcvData(String pk) {
        Uint8List? data = rcvRaw();
        if (data == null) {
            return null;
        }
        return unpackData( pk, data );
    }

    Uint8List? rcvRaw() {
        if ((_rsvstate != BinProtoRecv.data) &&
            (_rsvstate != BinProtoRecv.complete) &&
            (_rsvstate != BinProtoRecv.clear)) {
                return null;
            }
        int sz = _rcvsz;
        if (sz > _rcvBuf.length) {
            sz = _rcvBuf.length;
        }


        Uint8List data = Uint8List(sz);
        data.setAll(0, _rcvBuf.sublist(0, sz));
        _rcvBuf.removeRange(0, sz);
        _rcvsz -= sz;

        _rsvstate =
            _rcvsz > 0 ?
                BinProtoRecv.clear :
                BinProtoRecv.waitcmd;

        return data;
    }

    bool rcvNext() {
        // пропускаем данные по ожидаемой команде
        // и пытаемся принять следующую команду
        if ((_rsvstate != BinProtoRecv.data) &&
            (_rsvstate != BinProtoRecv.complete)) {
                return false;
            }
        
        if (_rcvsz > 0) {
            _rsvstate = BinProtoRecv.clear;
        }
        else {
            rcvClear();
        }
        
        return rcvProcess();
    }

    static List<int> _pklen(String pk, [int start = 0]) {
        int i = start;
        int sz = 0;
        int len = 0;
        
        while (
                (i < pk.length) &&
                (pk.codeUnitAt(i) >= '0'.codeUnitAt(0)) &&
                (pk.codeUnitAt(i) <= '9'.codeUnitAt(0))
            ) {
            len ++;
            sz = sz*10 + (pk.codeUnitAt(i)-'0'.codeUnitAt(0));
            i++;
        }

        if (len == 0) {
            return [1, 0];
        }

        return [sz, len];
    }

    static List<dynamic> unpackData(String pk, Uint8List data) {
        List<dynamic> vars = [];
        ByteData bdata = data.buffer.asByteData();

        int pi = 0;
        int di = 0;
        while (pi < pk.length) {
            switch (pk[pi]) {
                case ' ':
                    di ++;
                    break;
                
                case 'b':
                    vars.add(bdata.getInt8(di) != 0 ? true : false);
                    di ++;
                    break;
                
                case 'c':
                    vars.add(bdata.getInt8(di));
                    di ++;
                    break;

                case 'C':
                    vars.add(bdata.getUint8(di));
                    di ++;
                    break;
                
                case 'i':
                    vars.add(bdata.getInt16(di, Endian.big));
                    di += 2;
                    break;

                case 'I':
                    vars.add(bdata.getInt32(di, Endian.big));
                    di += 4;
                    break;
                
                case 'n':
                    vars.add(bdata.getUint16(di, Endian.big));
                    di += 2;
                    break;

                case 'N':
                case 'X':
                    vars.add(bdata.getUint32(di, Endian.big));
                    di += 4;
                    break;

                case 'H':
                    vars.add(bdata.getUint64(di, Endian.big));
                    di += 8;
                    break;
                
                case 'f':
                    double v = bdata.getInt16(di, Endian.big) / 100;
                    vars.add(v);
                    di += 2;
                    break;
                
                case 'D':
                    int vi = bdata.getInt32(di, Endian.big);
                    double vd = bdata.getUint32(di+4, Endian.big) / 0xffffffff;
                    vd += vi;
                    vars.add(vd);
                    di += 8;
                    break;

                case 'v':
                    vars.add(InternetAddress.fromRawAddress(data.sublist(di, di+4)));
                    di += 4;
                    break;

                case 'T':
                case 't':
                    final int ms = pk[pi] == 't' ? bdata.getUint8(di+7) : 0;
                    vars.add(
                        DateTime(
                            bdata.getUint16(di, Endian.big),
                            bdata.getUint8(di+2),
                            bdata.getUint8(di+3),
                            bdata.getUint8(di+4),
                            bdata.getUint8(di+5),
                            bdata.getUint8(di+6),
                            (ms >= 0) && (ms < 100) ? ms * 10 : 0
                        )
                    );
                    di += 8;
                    break;
                
                case 'a':
                    var v = _pklen(pk, pi+1);
                    int len = v[0];
                    if (v[1] > 0) {
                        pi += v[1];
                        final int len1 = data.length - di;
                        if (len > len1) len = len1;
                        for (int i = 0; i < len; i++) {
                            if (bdata.getUint8(di+i) == 0) {
                                len = i;
                                break;
                            }
                        }
                    }
                    vars.add(
                        String.fromCharCodes(data, di, di+len)
                    );
                    di += v[0];
                    break;
                
                case 's':
                    int len = bdata.getUint8(di);
                    final int len1 = data.length - di - 1;
                    if (len > len1) len = len1;
                    vars.add(
                        String.fromCharCodes(data, di+1, di+1+len)
                    );
                    di += len + 1;
                    break;
            }

            pi++;
        }

        return vars;
    }

    static Uint8List packData(String pk, List<dynamic> vars) {
        List<int> data = [];

        int pi = 0;
        int vi = 0;
        while (pi < pk.length) {
            var v = vars[vi];

            switch (pk[pi]) {
                case ' ':
                    data.add( 0 );
                    break;
                
                case 'b':
                    if (v is bool) {
                        data.add( v ? 1 : 0 );
                    }
                    else
                    if (v is int) {
                        data.add( v != 0 ? 1 : 0 );
                    }
                    else {
                        data.add( 0 );
                    }
                    break;

                case 'c':
                case 'C':
                    if (v is int) {
                        data.add( v );
                    }
                    else {
                        data.add( 0 );
                    }
                    break;

                case 'n':
                case 'x':
                    ByteData d = ByteData(2);
                    if (v is int) {
                        d.setUint16(0, v, Endian.big);
                    }
                    else {
                        d.setUint16(0, 0, Endian.big);
                    }
                    data.addAll( d.buffer.asInt8List() );
                    break;

                case 'i':
                    ByteData d = ByteData(2);
                    if (v is int) {
                        d.setInt16(0, v, Endian.big);
                    }
                    else {
                        d.setInt16(0, 0, Endian.big);
                    }
                    data.addAll( d.buffer.asInt8List() );
                    break;

                case 'N':
                case 'X':
                    ByteData d = ByteData(4);
                    if (v is int) {
                        d.setUint32(0, v, Endian.big);
                    }
                    else {
                        d.setUint32(0, 0, Endian.big);
                    }
                    data.addAll( d.buffer.asInt8List() );
                    break;

                case 'I':
                    ByteData d = ByteData(4);
                    if (v is int) {
                        d.setInt32(0, v, Endian.big);
                    }
                    else {
                        d.setInt32(0, 0, Endian.big);
                    }
                    data.addAll( d.buffer.asInt8List() );
                    break;
                
                case 'H':
                    ByteData d = ByteData(8);
                    if (v is int) {
                        d.setUint64(0, v, Endian.big);
                    }
                    else {
                        d.setUint64(0, 0, Endian.big);
                    }
                    data.addAll( d.buffer.asInt8List() );
                    break;
                
                case 'f':
                    ByteData d = ByteData(2);
                    if (v is double) {
                        d.setInt16(0, (v*100).round(), Endian.big);
                    }
                    else
                    if (v is int) {
                        d.setInt16(0, v*100, Endian.big);
                    }
                    else {
                        d.setInt16(0, 0, Endian.big);
                    }
                    data.addAll( d.buffer.asInt8List() );
                    break;
                
                case 'D':
                    ByteData d = ByteData(8);
                    if (v is double) {
                        int vi = v.ceil();
                        double vd = v-vi;
                        d.setInt32(0, vi, Endian.big);
                        d.setUint32(4, (vd * 0xffffffff).ceil(), Endian.big);
                    }
                    else
                    if (v is int) {
                        d.setInt32(0, v, Endian.big);
                        d.setUint32(4, 0, Endian.big);
                    }
                    else {
                        d.setUint64(0, 0, Endian.big);
                    }
                    data.addAll( d.buffer.asInt8List() );
                    break;
                
                case 'T':
                case 't':
                    ByteData d = ByteData(8);
                    if (v is DateTime) {
                        d.setUint16(0, v.year, Endian.big);
                        d.setUint8(2, v.month);
                        d.setUint8(3, v.day);
                        d.setUint8(4, v.hour);
                        d.setUint8(5, v.minute);
                        d.setUint8(6, v.second);
                        d.setUint8(7, pk[pi] == 't' ? (v.millisecond % 1000) ~/ 10 : 0);
                    }
                    else {
                        d.setUint64(0, 0, Endian.big);
                    }
                    data.addAll( d.buffer.asInt8List() );
                    break;
                
                case 'a':
                    var v = _pklen(pk, pi+1);
                    pi += v[1];
                    int len = v[0];
                    final String s = v is String ? v.toString() : '';
                    List<int> d = s.substring(0, len).codeUnits;
                    while (d.length < len) {
                        d.add(0);
                    }
                    data.addAll( d );
                    break;
                
                case 's':
                    String s = v is String ? v.toString() : '';
                    int len = s.length;
                    if (len > 255) len = 255;
                    List<int> d = s.substring(0, len).codeUnits;
                    while (d.length < len) {
                        d.add(0);
                    }
                    data.add(len);
                    data.addAll( d );
                    break;
            }

            if (pk[pi] != ' ') {
                vi ++;
            }
            pi++;
        }

        return Uint8List.fromList(data);
    }

    Uint8List pack(int cmd, [String? pk, List<dynamic>? vars]) {
        List<int> data = [mgcsnd, cmd, 0, 0];

        if ((pk != null) && (vars != null)) {
            data.addAll(packData(pk, vars));

            int sz = data.length - hdrsz;
            ByteData d = ByteData(2);
            d.setUint16(0, sz, Endian.big);
            data.setAll(2, d.buffer.asUint8List());
        }

        return Uint8List.fromList(data);
    }
}


typedef Struct = Map<String, dynamic>;

Struct fldUnpack(List<String> fld, List<dynamic> vars, [ int start = 0 ]) {
    int ifld = 0;
    int ivar = start;
    Map<String, dynamic> m = {};

    while ((ifld < fld.length) && (ivar < vars.length)) {
        m[ fld[ifld] ] = vars[ ivar ];
        ifld++;
        ivar++;
    }

    return m;
}

List<dynamic> fldPack(List<String> fld, Struct vars) {
    List<dynamic> list = [];

    for (var e in fld) {
        list.add( vars[e] );
    }

    return list;
}
