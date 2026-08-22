package com.gta.launcher.util;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.List;

public final class SampQuery {

    public static class Info {
        public boolean online;
        public boolean passworded;
        public int players;
        public int maxPlayers;
        public String hostname = "";
        public String gamemode = "";
        public String language = "";
        public long pingMs = -1;
    }

    public static class Player {
        public int id;
        public String name;
        public int score;
        public int ping;
    }

    private SampQuery() {}

    private static DatagramPacket build(String host, int port, char opcode) throws Exception {
        ByteBuffer b = ByteBuffer.allocate(11);
        b.order(ByteOrder.LITTLE_ENDIAN);
        b.put((byte)'S').put((byte)'A').put((byte)'M').put((byte)'P');
        String[] parts = host.split("\\.");
        for (String p : parts) b.put((byte) Integer.parseInt(p.trim()));
        b.putShort((short) port);
        b.put((byte) opcode);
        return new DatagramPacket(b.array(), b.position());
    }

    private static DatagramSocket send(String host, int port, char opcode,
                                       int timeoutMs, DatagramPacket out) throws Exception {
        DatagramSocket sock = new DatagramSocket();
        sock.setSoTimeout(timeoutMs);
        InetAddress addr = InetAddress.getByName(host);
        sock.send(new DatagramPacket(out.getData(), out.getLength(), addr, port));
        return sock;
    }

    public static Info queryInfo(String host, int port, int timeoutMs) {
        Info info = new Info();
        long t0 = System.currentTimeMillis();
        try {
            DatagramPacket req = build(host, port, 'i');
            DatagramSocket sock = send(host, port, 'i', timeoutMs, req);
            byte[] buf = new byte[2048];
            DatagramPacket resp = new DatagramPacket(buf, buf.length);
            sock.receive(resp);
            sock.close();
            info.pingMs = System.currentTimeMillis() - t0;

            ByteBuffer b = ByteBuffer.wrap(buf, 10, resp.getLength() - 10);
            b.order(ByteOrder.LITTLE_ENDIAN);
            if ((char) buf[10] != 'i') { info.online = true; return info; }
            info.online = true;
            info.passworded = b.getShort() != 0;
            info.players = b.getShort() & 0xFFFF;
            info.maxPlayers = b.getShort() & 0xFFFF;
            info.hostname = readStr(b);
            info.gamemode = readStr(b);
            info.language = readStr(b);
        } catch (Throwable ignored) {
            info.online = false;
        }
        return info;
    }

    public static List<Player> queryPlayers(String host, int port, int timeoutMs) {
        List<Player> list = new ArrayList<>();
        try {
            DatagramPacket req = build(host, port, 'd');
            DatagramSocket sock = send(host, port, 'd', timeoutMs, req);
            byte[] buf = new byte[4096];
            DatagramPacket resp = new DatagramPacket(buf, buf.length);
            sock.receive(resp);
            sock.close();

            ByteBuffer b = ByteBuffer.wrap(buf, 11, resp.getLength() - 11);
            b.order(ByteOrder.LITTLE_ENDIAN);
            int count = b.getShort() & 0xFFFF;
            for (int i = 0; i < count && b.remaining() > 9; i++) {
                Player p = new Player();
                p.id = b.get() & 0xFF;
                p.name = readStr(b);
                p.score = b.getInt();
                p.ping = b.getInt();
                list.add(p);
            }
        } catch (Throwable ignored) {}
        return list;
    }

    private static String readStr(ByteBuffer b) {
        int len = b.getInt();
        if (len < 0 || len > 512 || len > b.remaining()) return "";
        byte[] s = new byte[len];
        b.get(s);
        return new String(s);
    }
}
