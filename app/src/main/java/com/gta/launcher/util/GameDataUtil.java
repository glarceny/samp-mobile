package com.gta.launcher.util;

import org.ini4j.Wini;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

public final class GameDataUtil {
    public static final String STORAGE_ROOT = "/storage/emulated/0";
    public static final String VICE_DIR = STORAGE_ROOT + "/VICE";
    public static final String SAMP_DIR = STORAGE_ROOT + "/GTA/SAMP";
    public static final String SETTINGS_INI = SAMP_DIR + "/settings.ini";
    public static final String CACHE_URL =
            "https://github.com/glarceny/samp-mobile/releases/download/v0.9.0-base211/cache-repack.zip";

    private GameDataUtil() {}

    public static boolean isDataReady() {
        File models = new File(VICE_DIR, "models");
        File anim = new File(VICE_DIR, "anim/ped.ifp");
        File texdb = new File(VICE_DIR, "texdb");
        return models.isDirectory() && anim.isFile() && texdb.isDirectory();
    }

    public static boolean ensureSettingsDirs() {
        File d = new File(SAMP_DIR);
        if (!d.exists()) return d.mkdirs();
        return true;
    }

    public static String readNick(String fallback) {
        try {
            Wini ini = new Wini(new File(SETTINGS_INI));
            String n = ini.get("client", "name");
            if (n != null && !n.trim().isEmpty()) return n.trim();
        } catch (Throwable ignored) {}
        return fallback;
    }

    public static String readHost(String fallback) {
        try {
            Wini ini = new Wini(new File(SETTINGS_INI));
            String h = ini.get("client", "host");
            if (h != null && !h.trim().isEmpty()) return h.trim();
        } catch (Throwable ignored) {}
        return fallback;
    }

    public static int readPort(int fallback) {
        try {
            Wini ini = new Wini(new File(SETTINGS_INI));
            Integer p = ini.get("client", "port", int.class);
            if (p != null && p > 0) return p;
        } catch (Throwable ignored) {}
        return fallback;
    }

    public static void writeConnection(String nick, String host, int port, String password) throws IOException {
        if (!ensureSettingsDirs()) throw new IOException("Gagal membuat folder " + SAMP_DIR);
        Wini ini;
        File f = new File(SETTINGS_INI);
        if (f.exists()) {
            try {
                ini = new Wini(f);
            } catch (Throwable t) {
                ini = new Wini();
                seedDefaults(ini);
            }
        } else {
            ini = new Wini();
            seedDefaults(ini);
        }
        ini.put("client", "name", nick == null ? "Player" : nick);
        ini.put("client", "host", host);
        ini.put("client", "port", String.valueOf(port));
        if (password == null) password = "";
        ini.put("client", "password", password);
        ini.put("client", "version", "0.3.7");
        ini.put("client", "autoaim", "false");
        try {
            FileOutputStream fos = new FileOutputStream(f);
            ini.store(fos);
            fos.close();
        } catch (IOException e) {
            throw e;
        }
    }

    private static void seedDefaults(Wini ini) {
        ini.put("client", "name", "Player");
        ini.put("client", "host", "141.95.234.17");
        ini.put("client", "port", "7777");
        ini.put("client", "password", "");
        ini.put("client", "version", "0.3.7");
        ini.put("client", "autoaim", "false");
        ini.put("debug", "debug", "false");
        ini.put("debug", "online", "true");
        ini.put("gui", "Font", "arial.ttf");
    }

    public static void copyFile(File src, File dst) throws IOException {
        FileInputStream in = new FileInputStream(src);
        FileOutputStream out = new FileOutputStream(dst);
        byte[] buf = new byte[8192];
        int r;
        while ((r = in.read(buf)) > 0) out.write(buf, 0, r);
        in.close();
        out.close();
    }

    public static byte[] utf8(String s) { return s.getBytes(StandardCharsets.UTF_8); }
}
