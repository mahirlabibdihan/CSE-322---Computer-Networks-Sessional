package client;

import io.Color;
import io.Console;
import server.User;
import util.*;

import java.io.*;
import java.io.File;
import java.net.SocketTimeoutException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;


public class Client {
    NetworkUtil networkUtil;
    String username;

    public Client(String serverAddress, int serverPort) {
        while (true) {
            try {
                username = Console.getString("username");
                networkUtil = new NetworkUtil(serverAddress, serverPort);
                ReadBuffer.get().setNetworkUtil(networkUtil);
                JSON res = ReadBuffer.get().read();
                if (res.get("status").equals(Type.Response.CONNECT)) {
                    networkUtil.write(new JSON().
                            put("type", Type.Request.LOGIN).
                            put("username", username)
                    );
                } else {
                    Console.printError("Can't Connect to Server");
                }
                this.run();
            } catch (Exception e) {
                System.out.println(e.getMessage());
            }
        }
    }

    public void userFiles(String user) throws Exception {
        while (true) {
            JSON res = HTTP.get(new JSON().
                    put("type", Type.Request.PUBLIC_FILES).
                    put("user", user), networkUtil);
            ArrayList<String> files = (ArrayList) res.get("files");
            Console.printList(files);
            System.out.println("0. Back");
            int option = Console.getOption(0, files.size());
            if (option == 0) {
                return;
            } else {
                download(new FileObject(user, files.get(option - 1), "public"));
            }
        }
    }

    public void printUsers(List<Map.Entry<String, Boolean>> list) {
        if (list.isEmpty()) {
            Console.printError("Nothing to show");
        }
        for (int i = 0; i < list.size(); i++) {
            if (list.get(i).getValue()) {
                System.out.println(Color.YELLOW + (i + 1) + ". " + list.get(i).getKey() + Color.RESET);
            } else {
                System.out.println((i + 1) + ". " + list.get(i).getKey());
            }
        }
    }

    public void userList(List<Map.Entry<String, Boolean>> list) throws Exception {
        while (true) {
            printUsers(list);
            System.out.println("0. Back");
            int option = Console.getOption(0, list.size());
            if (option == 0) {
                break;
            } else {
                userFiles(list.get(option - 1).getKey());
            }
        }
    }


    public void printProgress(int progress) {
        if (progress < 10) {
            System.out.print("\b\b");
        } else if (progress <= 100) {
            System.out.print("\b\b\b");
        }
        System.out.print(progress + "%");
    }

    public Boolean send(FileObject file, int chunk_size) throws Exception {
        file.setContent();
        System.out.print("Uploading------------------------------0%");
        List<byte[]> chunks = FileUtil.splitFile(file.getContent(), chunk_size);
        long total = file.getSize(), curr = 0;
        for (byte[] chunk : chunks) {
            networkUtil.write(new JSON().
                    put("type", Type.Request.UPLOAD_CHUNK).
                    put("chunk", chunk));
            JSON res = ReadBuffer.get().getAcknowledge();
            curr += chunk.length;
            if (res != null) {
                printProgress((int) ((1.0 * curr / total) * 100));
            } else {
                System.out.println();
                Console.printError("Unexpected Error");
                return false;
            }
        }
        return true;
    }

    public void upload(String filetype, int req_id) throws Exception {
        String path = Console.getFilePath();
        FileObject file = new FileObject(path, filetype);
        JSON res = HTTP.post(new JSON().
                        put("type", Type.Request.UPLOAD_FILE).
                        put("req_id", req_id).
                        put("file", file),
                networkUtil);

        networkUtil.setTimeout();
        if (res.get("status").equals(Type.Response.SUCCESS)) {
            int chunk_size = (int) res.get("chunk_size");
            try {
                if (!send(file, chunk_size)) {
                    return;
                }
                res = HTTP.post(new JSON().put("type", Type.Request.COMPLETE), networkUtil);
                if (res.get("status").equals(Type.Response.COMPLETE)) {
                    System.out.println();
                    Console.printSuccess("File Uploaded");
                } else if (res.get("status").equals(Type.Response.ERROR)) {
                    System.out.println();
                    Console.printError("Try Again");
                }
            } catch (SocketTimeoutException e) {
                System.out.println();
                Console.printError("TIMEOUT");
                networkUtil.write(new JSON().put("type", Type.Request.TIMEOUT));
                // Timeout but an ACK will eventually come. What about that?
            }
        } else {
            Console.printError("Transmission not allowed");
        }
        networkUtil.resetTimeout();
    }

    public void download(FileObject file) throws Exception {
        networkUtil.write(new JSON().
                put("type", Type.Request.DOWNLOAD_FILE).
                put("file", file)
        );
        ArrayList<byte[]> chunk_list = new ArrayList<>();
        System.out.print("Downloading----------------------------0%");
        while (true) {
            JSON res = ReadBuffer.get().read();
            if (res.get("status").equals(Type.Response.COMPLETE)) {
                String filedir = "downloads/" + username + "/";
                new File(filedir).mkdirs();
                System.out.println();
                String name = Console.getLine("Save as");
                FileOutputStream fos = new FileOutputStream(filedir + name, false);
                for (byte[] bytes : chunk_list) {
                    fos.write(bytes);
                    fos.flush();
                }
                fos.close();
                break;
            } else {
                byte[] chunk = (byte[]) res.get("chunk");
                chunk_list.add(chunk);
                printProgress((int) res.get("progress"));
            }
        }
        System.out.println();
        Console.printSuccess("File Downloaded");
    }

    public void myFiles(String access) throws Exception {
        while (true) {
            JSON res = HTTP.get(new JSON().
                    put("type", Type.Request.MY_FILES).
                    put("filetype", access), networkUtil);


            List<String> files = (ArrayList<String>) res.get("files");
            int n = files.size();
            Console.printList(files);
            System.out.println((n + 1) + ". Upload File");
            System.out.println("0. Back");
            int option = Console.getOption(0, n + 1);
            if (option == 0) {
                return;
            } else if (option == n + 1) {
                upload(access, -1);
            } else {
                // Download
                download(new FileObject(username, files.get(option - 1), access));
            }
        }
    }

    public void requestFile() throws Exception {
        String description = Console.getLine("File Description");
        JSON res = HTTP.post(new JSON().
                        put("type", Type.Request.REQUEST_FILE).
                        put("desc", description),
                networkUtil);
        if (res.get("status").equals(Type.Response.SUCCESS)) {
            Console.printSuccess("Request id: " + res.get("req_id"));
        }
    }

    public void fileRequests() throws Exception {
        while (true) {
            JSON res = HTTP.get(new JSON().
                    put("type", Type.Request.SHOW_REQUESTS), networkUtil);
            List<Message> list = (ArrayList) res.get("list");
            Console.printList(list);
            System.out.println("0. Back");
            int option = Console.getOption(0, list.size());
            switch (option) {
                case 0:
                    return;
                default:
                    upload("public", list.get(option - 1).getId());
                    break;
            }
        }
    }

    public void fileMenu() throws Exception {
        while (true) {
            System.out.println("1. Private" + "\n" +
                    "2. Public" + "\n" +
                    "0. Back");
            int option = Console.getOption(0, 2);
            switch (option) {
                case 1:
                    myFiles("private");
                    break;
                case 2:
                    myFiles("public");
                    break;
                case 0:
                    return;
            }
        }
    }

    public Boolean logout() throws Exception {
        JSON res = HTTP.get(new JSON().put("type", Type.Request.LOGOUT), networkUtil);
        if (res.get("status").equals(Type.Response.SUCCESS)) {
            Console.printSuccess("You are logged out");
            return true;
        } else {
            Console.printError("Can't log out");
        }
        return false;
    }

    public void showUsers() throws Exception {
        JSON res = HTTP.get(new JSON().put("type", Type.Request.SHOW_USERS), networkUtil);
        if (res.get("status").equals(Type.Response.SUCCESS)) {
            userList((ArrayList) res.get("list"));
        } else {
            Console.printError("Can't fetch list");
        }
    }

    public void mainMenu() throws Exception {
        while (true) {
            System.out.println("1. Users" + "\n" +
                    "2. Request File" + "\n" +
                    "3. Files" + "\n" +
                    "4. Messages" + "\n" +
                    "5. Requests" + "\n" +
                    "6. Log Out");

            int option = Console.getOption(1, 6);
            JSON res;
            switch (option) {
                case 1:
                    showUsers();
                    break;
                case 2:
                    requestFile();
                    break;
                case 3:
                    fileMenu();
                    break;
                case 4:
                    messageMenu();
                    break;
                case 5:
                    fileRequests();
                    break;
                case 6:
                    if (logout()) return;
                    break;
            }
        }
    }

    private void messageMenu() throws Exception {
        List<Message> list = new ArrayList<>();
        while (true) {
            JSON res = HTTP.get(new JSON().
                            put("type", Type.Request.SHOW_MSG),
                    networkUtil);
            list.addAll((ArrayList) res.get("msg"));
            Console.printList(list);
            System.out.println("0. Back");
            int option = Console.getOption(0, list.size());
            if (option == 0) {
                return;
            } else {
                Message msg = list.get(option - 1);
                if (msg.getType().equals(Type.Message.FILE_REQUEST)) {
                    // upload
                    upload("public", msg.getId());
                } else if (msg.getType().equals(Type.Message.FILE_UPLOAD)) {
                    // download
                    download(new FileObject(msg.getFrom(), msg.getFilename(), "public"));
                }
            }
        }
    }

    public void run() {
        try {
            JSON res = ReadBuffer.get().read();
            if (res.get("status").equals(Type.Response.SUCCESS)) {
                mainMenu();
            }
        } catch (Exception e) {
            Console.printError("You are already logged in");
        } finally {
            try {
                networkUtil.closeConnection();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }


    public static void main(String args[]) {
        new Client("localhost", 5000);
    }
}
