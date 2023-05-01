
import 'dart:io';
import 'dart:typed_data';

class cks8 {
    int _cka;
    int _ckb;
    int get a => _cka;
    int get b => _ckb;

    cks8([this._cka = 0, this._ckb = 0]);

    void add(int byte) {
        _cka = (_cka + byte) & 0xff;
        _ckb = (_ckb + _cka) & 0xff;
    }
    cks8 operator+(int byte) {
        int cka1 = (_cka + byte) & 0xff;
        return cks8(cka1, (_ckb + cka1) & 0xff);
    }
    void addlist(Uint8List data) {
        for (final byte in data) {
            add(byte);
        }
    }
    void reset() {
        _cka = 0;
        _ckb = 0;
    }
}

class FileBin {
    RandomAccessFile ?fh;
    FileBin([String ?file]) {
        if (file != null) {
            fh = File(file).openSync();
        }
    }

    bool get isOpened => fh != null;
    bool operator () => fh != null;

    int  get pos => fh == null ? 0 : fh!.positionSync();
    int  get size=> fh == null ? 0 : fh!.lengthSync();
    bool get eof => (fh == null) || (fh!.positionSync() >= fh!.lengthSync());
    
    Future<bool> open(String file) async {
        fh = await File(file).open();
        return fh != null;
    }

    Future<List<dynamic>?> get(String pk) async {
        final data = await getdata();
        if (data == null) {
            return null;
        }

        return unpack(pk, data);
    }

    Future<Uint8List ?> getdata() async {
        int sz = 0;
        final ck = cks8();

        if (fh == null) return null;
        
        final dsz = await fh?.read(2);
        if ((dsz == null) || (dsz.length != 2)) {
            return null;
        }
        ck.addlist(dsz);
        
        sz |= dsz[0];
        sz |= dsz[1] << 8;

        final data = await fh!.read(sz);
        if (data.length != sz) {
            return null;
        }
        ck.addlist(data);

        final dck = await fh!.read(2);
        if (dck.length != 2) {
            return null;
        }
        
        if ((dck[0] != ck.a) || (dck[1] != ck.b))
            return null;
        
        return data;
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
    
    static List<dynamic> unpack(String pk, Uint8List data) {
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
                    vars.add(bdata.getInt16(di, Endian.little));
                    di += 2;
                    break;

                case 'I':
                    vars.add(bdata.getInt32(di, Endian.little));
                    di += 4;
                    break;
                
                case 'n':
                    vars.add(bdata.getUint16(di, Endian.little));
                    di += 2;
                    break;

                case 'N':
                case 'X':
                    vars.add(bdata.getUint32(di, Endian.little));
                    di += 4;
                    break;

                case 'H':
                    vars.add(bdata.getUint64(di, Endian.little));
                    di += 8;
                    break;
                
                case 'f':
                    double v = bdata.getInt16(di, Endian.little) / 100;
                    vars.add(v);
                    di += 2;
                    break;
                
                /*
                case 'D':
                    int vi = bdata.getInt32(di, Endian.big);
                    double vd = bdata.getUint32(di+4, Endian.big) / 0xffffffff;
                    vd += vi;
                    vars.add(vd);
                    di += 8;
                    break;
                */

                case 'v':
                    vars.add(InternetAddress.fromRawAddress(data.sublist(di, di+4)));
                    di += 4;
                    break;

                case 'T':
                case 't':
                    final int ms = pk[pi] == 't' ? bdata.getUint8(di+7) : 0;
                    vars.add(
                        DateTime(
                            bdata.getUint16(di, Endian.little),
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
                
                /*
                case 's':
                    int len = bdata.getUint8(di);
                    final int len1 = data.length - di - 1;
                    if (len > len1) len = len1;
                    vars.add(
                        String.fromCharCodes(data, di+1, di+1+len)
                    );
                    di += len + 1;
                    break;
                
                case 'B':
                case 'Z':
                    int len;
                    if (pk[pi] == 'B') {
                        len = bdata.getUint16(di, Endian.big);
                        di += 2;
                    }
                    else {
                        len = bdata.getUint32(di, Endian.big);
                        di += 4;
                    }
                    final int len1 = data.length - di;
                    if (len > len1) len = len1;
                    vars.add(
                        data.sublist(di, di+len)
                    );
                    di += len;
                    break;
                */
            }

            pi++;
        }

        return vars;
    }
}