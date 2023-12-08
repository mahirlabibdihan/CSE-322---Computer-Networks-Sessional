package client;

import util.JSON;
import util.NetworkUtil;

import java.util.HashMap;
import java.util.Map;

public class HTTP {
    public static JSON get(Object req, NetworkUtil networkUtil) throws Exception {
        networkUtil.write(req);
        return ReadBuffer.get().read();
    }

    public static JSON post(Object body, NetworkUtil networkUtil) throws Exception {
        networkUtil.write(body);
        return ReadBuffer.get().read();
    }
}
