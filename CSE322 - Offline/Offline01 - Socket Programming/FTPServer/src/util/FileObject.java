package util;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.Serializable;

public class FileObject implements Serializable {
    private String name;
    private String type;
    private long size;
    private String id;
    private String owner;
    private byte[] content;
    private String path;

    public FileObject() {}

    public FileObject(String path, String type) {
        File file = new File(path);
        this.path = path;
        this.type = type;
        this.size = file.length();
        this.name = file.getName();
    }

    public FileObject(String owner, String name, String type) {
        this.owner = owner;
        this.name = name;
        this.type = type;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public long getSize() {
        return size;
    }

    public void setSize(long size) {
        this.size = size;
    }

    public String getId() {
        return id;
    }

    public void setId(String id) {
        this.id = id;
    }

    public String getOwner() {
        return owner;
    }

    public void setOwner(String owner) {
        this.owner = owner;
    }

    public byte[] getContent() {
        return content;
    }

    public void setContent() throws IOException {
        FileInputStream fis = new FileInputStream(path);
        content = fis.readAllBytes();
        fis.close();
    }

    public String getPath() {
        return path;
    }

    public void setPath(String path) {
        this.path = path;
        File file = new File(path);
    }
    public void generateId() {
        this.id =  "file_" + System.currentTimeMillis();
    }
}
