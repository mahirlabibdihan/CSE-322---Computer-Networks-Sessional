package util;

public class Type {
    public enum Response {
        CONNECT,
        ACK,
        COMPLETE,
        ERROR,
        SUCCESS
    }
    public enum Request {
        LOGIN,
        LOGOUT,
        UPLOAD_FILE,
        DOWNLOAD_FILE,
        REQUEST_FILE,
        SHOW_REQUESTS,
        SHOW_USERS,
        MY_FILES,
        PUBLIC_FILES,
        SHOW_MSG,
        UPLOAD_CHUNK,
        COMPLETE,
        TIMEOUT,
        SUCCESS,
        ERROR
    }
    public enum Message {
        FILE_REQUEST,
        FILE_UPLOAD
    }
}
