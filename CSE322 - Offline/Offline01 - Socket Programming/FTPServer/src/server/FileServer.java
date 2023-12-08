package server;

import io.Console;
import util.*;

import java.util.ArrayList;

public class FileServer {
    public static Boolean addChunkToBuffer(byte[] chunk, String file_id) {
        try {
            Buffer.get().addChunk(chunk, file_id);
            return true;
        } catch (Exception e) {
            return false;
        }
    }

    public static Boolean receive(NetworkUtil networkUtil, String file_id) throws Exception {
        while (true) {
            Object o = networkUtil.read();
            if (o instanceof JSON) {
                JSON req = (JSON) o;
                if (req.get("type").equals(Type.Request.UPLOAD_CHUNK)) {
//                    Thread.sleep((int)1.1*_CONFIG_.SOCKET_TIMEOUT); // To demonstrate timeout
                    if (addChunkToBuffer((byte[]) req.get("chunk"), file_id)) {
                        networkUtil.write(new JSON().put("status", Type.Response.ACK));
                    }
                } else if (req.get("type").equals(Type.Request.COMPLETE)) {
                    System.out.println("File Recieved");
                    break;
                } else if (req.get("type").equals(Type.Request.TIMEOUT)) {
                    Console.printError("TIMEOUT");
                    Buffer.get().removeChunks(file_id);
                    return false;
                }
            } else {
                networkUtil.write(new JSON().put("status", Type.Response.ERROR));
                Console.printError(o.getClass().getSimpleName());
                return false;
            }
        }
        return true;
    }

    public static void send(NetworkUtil networkUtil, JSON req) throws Exception {
        FileObject file = (FileObject) req.get("file");
        file.setPath("storage/" + file.getOwner() + "/" + file.getType() + "/" + file.getName());
        file.setContent();
        ArrayList<byte[]> chunk_list = FileUtil.splitFile(file.getContent(), _CONFIG_.MAX_CHUNK_SIZE);
        int total = 0, curr = 0;
        for (byte[] bytes : chunk_list) {
            total += bytes.length;
        }
        for (byte[] bytes : chunk_list) {
            curr += bytes.length;
            networkUtil.write(new JSON().
                    put("status", Type.Response.SUCCESS).
                    put("chunk", bytes).
                    put("progress", (int) ((1.0 * curr / total) * 100)));
        }
        networkUtil.write(new JSON().
                put("status", Type.Response.COMPLETE));
    }
}
