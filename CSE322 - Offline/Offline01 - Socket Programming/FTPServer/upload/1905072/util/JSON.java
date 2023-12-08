package util;

import java.io.Serializable;
import java.util.HashMap;
import java.util.Map;

public class JSON implements Serializable {
    Map<String, Object> json;

    public JSON() {
        json = new HashMap<>();
    }

    public JSON put(String key, Object value) {
        json.put(key, value);
        return this;
    }

    public Object get(String key) {
        return json.get(key);
    }
}
