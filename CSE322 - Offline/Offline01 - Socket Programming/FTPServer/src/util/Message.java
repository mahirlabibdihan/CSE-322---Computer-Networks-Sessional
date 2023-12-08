package util;

import java.io.Serializable;

public class Message implements Serializable {
    private String from;
    private String description;
    private int id;
    private String filename;
    private Type.Message type;

    public Message(String from, String description, int req_id, Type.Message type) {
        this.from = from;
        this.description = description;
        this.id = req_id;
        this.type = type;
    }

    public Type.Message getType() {
        return type;
    }

    public void setType(Type.Message type) {
        this.type = type;
    }

    public int getId() {
        return id;
    }

    public void setId(int id) {
        this.id = id;
    }

    public String getFilename() {
        return description;
    }

    public void setFilename(String filename) {
        this.description = description;
    }

    public String getFrom() {
        return from;
    }

    public void setFrom(String from) {
        this.from = from;
    }

    @Override
    public String toString() {
        if (type.equals(Type.Message.FILE_REQUEST)) {
            return (from + "> Requested " + description + " #" + id);
        } else if (type.equals(Type.Message.FILE_UPLOAD)) {
            return (from + "> Uploaded " + description + " #" + id);
        }
        return "";
    }
}
