package server;

import util.Message;
import util.NetworkUtil;

import java.util.ArrayList;
import java.util.List;

public class User {
    private NetworkUtil networkUtil;
    private String name;
    private List<Message> messages;
    private Boolean is_online;

    public User(String name) {
        this.name = name;
        this.messages = new ArrayList<>();
        this.is_online = false;
    }

    public void login(NetworkUtil networkUtil) {
        is_online = true;
        this.networkUtil = networkUtil;
    }

    public void logout() {
        is_online = false;
        this.networkUtil = null;
    }

    public Boolean isOnline(){
        return is_online;
    }
    public List<Message> readMessages() {
        List<Message> tmp = new ArrayList<>(messages);
        messages.clear();
        return tmp;
    }
    public void addMessage(Message message) {
        System.out.println(message);
        messages.add(message);
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public NetworkUtil getNetworkUtil() {
        return networkUtil;
    }

    public void setNetworkUtil(NetworkUtil networkUtil) {
        this.networkUtil = networkUtil;
    }
}
