
import 'dart:typed_data';
import 'dart:io';
import 'package:flutter_file_dialog/flutter_file_dialog.dart';
import 'package:file_picker/file_picker.dart';
import 'dart:developer' as developer;

Future<bool> trackSaveGpx({String? filename, required String data}) async {
    if (filename == null) filename = 'track.gpx';
    
    if (Platform.isAndroid || Platform.isIOS) {
        final filePath = await FlutterFileDialog.saveFile(
            params: SaveFileDialogParams(
                fileName: filename,
                //sourceFilePath: '~'
                data: Uint8List.fromList(data.codeUnits)
            )
        );
        developer.log('file: $filePath');
        return filePath != null;
    }
    else
    if (Platform.isLinux || Platform.isWindows || Platform.isMacOS) {
        final filePath = await FilePicker.platform.saveFile(
            dialogTitle: 'Please select an output file:',
            fileName: filename,
        );
        developer.log('file: $filePath');
        if (filePath == null) {
            return false;
        }

        final file = File(filePath);

        file.writeAsString(data);

        return true;
    }

    return false;
}