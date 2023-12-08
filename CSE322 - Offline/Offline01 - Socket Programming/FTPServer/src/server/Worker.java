package server;

import util.*;

import java.io.*;
import java.io.File;
import java.util.*;
import java.util.concurrent.ThreadLocalRandom;

public class Worker implements Runnable {
    private NetworkUtil networkUtil;
    private Thread thr;
    private User user;


    public Worker(NetworkUtil networkUtil) {
        this.user = null;
        this.networkUtil = networkUtil;
        this.thr = new Thread(this);
        thr.start();
    }

    public Message getRequest(int id) {
        for (Message r : Database.get().getRequests()) {
            if (r.getId() == id) {
                return r;
            }
        }
        return null;
    }

    private int generateChunkSize() {
        return ThreadLocalRandom.current().nextInt(_CONFIG_.MIN_CHUNK_SIZE, _CONFIG_.MAX_CHUNK_SIZE + 1);
    }

    private int getTotalChunkSize(List<byte[]> chunks) {
        int totalSize = 0;
        for (byte[] chunk : chunks) {
            totalSize += chunk.length;
        }
        return totalSize;
    }

    public void print(Object str) throws NullPointerException {
        System.out.println(user.getName() + "> " + str);
    }

    public void saveFile(FileObject file, int req_id) throws Exception {
        List<byte[]> chunks = Buffer.get().getChunks(file.getId());

        if (getTotalChunkSize(chunks) != file.getSize()) {
            // Not allowed
            networkUtil.write(new JSON().
                    put("status", Type.Response.ERROR));
        } else {
            String filePath = "storage/" + user.getName() + "/" + file.getType() + "/" + Database.get().getFileName(file.getId());
            FileOutputStream fos = new FileOutputStream(filePath, false);
            for (byte[] bytes : chunks) {
                fos.write(bytes);
                fos.flush();
            }
            fos.close();
            networkUtil.write(new JSON().put("status", Type.Response.COMPLETE));

            if (req_id >= 0) {
                String name = getRequest(req_id).getFrom();
                Database.get().getUsers().get(name).addMessage(
                        new Message(user.getName(), file.getName(), req_id, Type.Message.FILE_UPLOAD)
                );
            }
        }
    }


    public void receiveFile(JSON req) throws Exception {
        int req_id = (int) req.get("req_id");
        FileObject file = (FileObject) req.get("file");
        print("File Upload Requested");

        if (Buffer.get().getTotalSize() + file.getSize() <= Buffer.get().getMaxSize()) {
            int chunk_size = generateChunkSize();
            file.generateId();
            Database.get().addFileName(file.getId(), file.getName());
            print("File id: " + file.getId() + ", Chunk size: " + chunk_size);
            // Initiate Upload
            networkUtil.write(new JSON().
                    put("status", Type.Response.SUCCESS).
                    put("chunk_size", chunk_size).
                    put("file_id", file.getId()));

            // Client will start sending chunks
            print("Started Sending");
            if (FileServer.receive(networkUtil, file.getId())) {
                saveFile(file, req_id); // Save file locally
                Buffer.get().removeChunks(file.getId()); // Remove <FileId,Chunks>
            }
            Database.get().removeFileName(file.getId()); // Remove <FileId,Filename>
        } else {
            networkUtil.write(new JSON().
                    put("status", Type.Response.ERROR));
        }
    }

    public void fileRequest(String description) throws IOException {
        Message tmp_msg = new Message(user.getName(), description,
                Database.get().getRequests().size(), Type.Message.FILE_REQUEST);
        Database.get().addRequest(tmp_msg);
        for (Map.Entry<String, User> u : Database.get().getUsers().entrySet()) {
            if (!u.getKey().equals(user.getName())) {
                print("Sent message to " + u.getKey());
                u.getValue().addMessage(tmp_msg);
            }
        }
        networkUtil.write(new JSON().
                put("status", Type.Response.SUCCESS).
                put("req_id", tmp_msg.getId()));
    }

    public Boolean login(String name) throws IOException {
        if (Database.get().getUsers().containsKey(name)) {
            User u = Database.get().getUsers().get(name);
            if (u.isOnline()) return false;
            user = u;
        } else {
            user = new User(name);
            new File("storage/" + user.getName() + "/public").mkdirs();
            new File("storage/" + user.getName() + "/private").mkdirs();
        }
        user.login(networkUtil);
        Database.get().addUser(user);
        networkUtil.write(new JSON().put("status", Type.Response.SUCCESS));
        print("logged in");
        return true;
    }

    public void loop() {
        try {
            while (true) {
                Object o = networkUtil.read();
                if (o instanceof JSON) {
                    JSON req = (JSON) o;
                    print(req.get("type"));
                    switch ((Type.Request) req.get("type")) {
                        case LOGOUT:
                            print("logged out");
                            user.logout();
                            networkUtil.write(new JSON().put("status", Type.Response.SUCCESS));
                            throw new Exception();
                        case SHOW_USERS:
                            List<Map.Entry<String, Boolean>> list = new ArrayList<>();
                            for (Map.Entry<String, User> u : Database.get().getUsers().entrySet()) {
                                if (!user.getName().equals(u.getKey())) {
                                    list.add(new AbstractMap.SimpleEntry<>(u.getKey(), u.getValue().isOnline()));
                                }
                            }
                            networkUtil.write(new JSON().
                                    put("status", Type.Response.SUCCESS).
                                    put("list", list));
                            break;

                        case SHOW_REQUESTS:
                            List<Message> reqs = new ArrayList<>();
                            for (Message m : Database.get().getRequests()) {
                                if (!m.getFrom().equals(user.getName())) {
                                    reqs.add(m);
                                }
                            }
                            networkUtil.write(new JSON().
                                    put("status", Type.Response.SUCCESS).
                                    put("list", reqs));
                            break;

                        case MY_FILES:
                            String path = "storage/" + user.getName() + "/" + req.get("filetype") + "/";
                            File folder = new File(path);
                            File a[] = folder.listFiles();
                            List<String> files = new ArrayList<>();
                            for (File f : a) {
                                files.add(f.getName());
                            }
                            networkUtil.write(new JSON().
                                    put("status", Type.Response.SUCCESS).
                                    put("files", files));
                            break;
                        case PUBLIC_FILES:
                            folder = new File("storage/" + req.get("user") + "/" + "public" + "/");
                            a = folder.listFiles();
                            files = new ArrayList<>();
                            for (File f : a) {
                                files.add(f.getName());
                            }
                            networkUtil.write(new JSON().
                                    put("status", Type.Response.SUCCESS).
                                    put("files", files));
                            break;
                        case REQUEST_FILE:
                            fileRequest((String) req.get("desc"));
                            break;
                        case SHOW_MSG:
                            networkUtil.write(new JSON().
                                    put("status", Type.Response.SUCCESS).
                                    put("msg", user.readMessages()));
                            break;
                        case DOWNLOAD_FILE:
                            FileServer.send(networkUtil, req);
                            break;
                        case UPLOAD_FILE:
                            receiveFile(req);
                            break;
                    }
                }
            }
        } catch (Exception e) {
            print("Socket disconnected");
        } finally {
            try {
                print("Logged out");
                networkUtil.closeConnection();
                user.logout();
            } catch (IOException e) {
                System.out.println("Not logged in");
            }
        }
    }

    @Override
    public void run() {
        try {
            // Client Connected
            networkUtil.write(new JSON().put("status", Type.Response.CONNECT));
            System.out.println("New client connect");
            // Client Log In
            JSON req = (JSON) networkUtil.read();
            if (req.get("type") == Type.Request.LOGIN) {
                if (!login((String) req.get("username"))) {
                    networkUtil.closeConnection();
                    return;
                }
            }
        } catch (Exception e) {
        }
        loop();
    }
}
