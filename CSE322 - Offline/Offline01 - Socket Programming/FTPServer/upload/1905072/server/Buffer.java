package server;

import java.util.*;

public class Buffer {
    private static final Buffer instance = new Buffer();
    private Map<String,List<byte[]>> fileIDToChunkMap;
    private int maxSize;
    private int size;

    private Buffer() {
        this.size = 0;
        this.maxSize = _CONFIG_.MAX_BUFFER_SIZE;
        this.fileIDToChunkMap = new LinkedHashMap<>();
    }

    public static Buffer get() {
        return instance;
    }

    public synchronized boolean isFull() {
        return getTotalSize() >= maxSize;
    }

    public synchronized int getTotalSize() {
        return size;
    }

    public synchronized void addChunk(byte[] chunk, String fileID) throws Exception {
        if (chunk.length + size > maxSize) {
            throw new Exception();
        }
        size += chunk.length;
        if(fileIDToChunkMap.containsKey(fileID))
        {
            fileIDToChunkMap.get(fileID).add(chunk);
        }
        else
        {
            List<byte[]> list = new ArrayList<>();
            list.add(chunk);
            fileIDToChunkMap.put(fileID,list);
        }
    }

    public synchronized List<byte[]> getChunks(String fileID) {
        return new ArrayList<>(fileIDToChunkMap.get(fileID));
    }

    public synchronized void removeChunks(String fileID) {
        while (!fileIDToChunkMap.get(fileID).isEmpty())
        {
            size -= fileIDToChunkMap.get(fileID).get(0).length;
            fileIDToChunkMap.get(fileID).remove(0);
        }
        fileIDToChunkMap.remove(fileID);
    }
    public int getMaxSize() {
        return maxSize;
    }
}

