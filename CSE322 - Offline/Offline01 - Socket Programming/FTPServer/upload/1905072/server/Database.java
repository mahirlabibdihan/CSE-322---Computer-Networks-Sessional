package server;

import util.Message;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class Database {
    private static final Database instance = new Database();
    private int buffer_size;
    private List<Message> requests;
    public Map<String, String> fileIDToName;
    private Map<String, User> users;

    private Database() {
        this.buffer_size = 0;
        this.fileIDToName = new HashMap<>();
        this.requests = new ArrayList<>();
        this.users = new HashMap<>();
    }

    public static Database get() {
        return instance;
    }

    public void addFileName(String fileid, String filename) {
        fileIDToName.put(fileid, filename);
    }

    public String getFileName(String fileid) {
        return fileIDToName.get(fileid);
    }

    public void removeFileName(String fileid) {
        fileIDToName.remove(fileid);
    }

    public List<Message> getRequests() {
        return requests;
    }

    public void addRequest(Message r) {
        this.requests.add(r);
    }

    public Map<String, User> getUsers() {
        return users;
    }

    public void addUser(User u) {
        this.users.put(u.getName(), u);
    }
}
