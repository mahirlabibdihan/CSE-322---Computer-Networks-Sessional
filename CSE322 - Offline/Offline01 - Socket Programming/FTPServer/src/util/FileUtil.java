package util;

import server.Buffer;
import server.Database;

import java.io.FileOutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class FileUtil {
    public static ArrayList<byte[]> splitFile(byte[] fileContent, int chunkSize) {
        int start = 0;
        int fileSize = fileContent.length;
        ArrayList<byte[]> chunkList = new ArrayList<>();

        while (start < fileSize) {
            int end = Math.min(fileSize, start + chunkSize);
            chunkList.add(Arrays.copyOfRange(fileContent, start, end));
            start += chunkSize;
        }
        return chunkList;
    }
}
