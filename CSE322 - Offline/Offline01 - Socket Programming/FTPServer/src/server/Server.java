package server;

import util.NetworkUtil;

import java.io.File;
import java.net.ServerSocket;
import java.net.Socket;

public class Server {
    public static void deleteFolder(File folder) {
        File[] files = folder.listFiles();
        if (files != null) { //some JVMs return null for empty dirs
            for (File f : files) {
                if (f.isDirectory()) {
                    deleteFolder(f);
                } else {
                    f.delete();
                }
            }
        }
        folder.delete();
    }

    Server() {
        try {
            ServerSocket serverSocket = new ServerSocket(5000);
            System.out.println("Server opened at port 5000");
            deleteFolder(new File("storage"));
            while (true) {
                Socket clientSocket = serverSocket.accept();
                serve(clientSocket);
            }
        } catch (Exception e) {
            System.out.println(e);
        }
    }


    public void serve(Socket clientSocket) throws Exception {
        NetworkUtil networkUtil = new NetworkUtil(clientSocket);
        new Worker(networkUtil);
    }

    public static void main(String args[]) {
        new Server();
    }
}