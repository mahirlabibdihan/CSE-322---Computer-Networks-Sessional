package client;

import util.JSON;
import util.NetworkUtil;
import util.Type;

public class ReadBuffer {
    private NetworkUtil networkUtil;
    private JSON res;
    private static ReadBuffer instance = new ReadBuffer();

    private ReadBuffer() {
        res = null;
        networkUtil = null;
    }

    public NetworkUtil getNetworkUtil() {
        return networkUtil;
    }

    public void setNetworkUtil(NetworkUtil networkUtil) {
        this.networkUtil = networkUtil;
    }

    public static ReadBuffer get() {
        return instance;
    }

    public JSON getAcknowledge() throws Exception {
        JSON ret;
        if (res == null) {
            res = (JSON) networkUtil.read();
        }
        if(!res.get("status").equals(Type.Response.ACK)){
            return null;
        }
        ret = res;
        res = null;
        return ret;
    }
    public JSON read() throws Exception { // Non acknowledge read
        JSON ret;
        if (res == null || res.get("status").equals(Type.Response.ACK)) {
            while (true){
                res = (JSON) networkUtil.read();
                if(!res.get("status").equals(Type.Response.ACK)) {
                    break;
                } // Or else it's an acknowledgement from previous failed upload
            }
        }
        ret = res;
        res = null;
        return ret;
    }

    public void unread(JSON res) {
        this.res = res;
    }
}
