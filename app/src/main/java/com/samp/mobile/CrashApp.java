package com.samp.mobile;

import android.app.Application;
import android.content.Intent;
import android.util.Log;

import java.io.File;
import java.io.FileWriter;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

public class CrashApp extends Application {
    public static final String CRASH_FILE = "crash_log.txt";

    @Override
    public void onCreate() {
        super.onCreate();
        final Thread.UncaughtExceptionHandler prev = Thread.getDefaultUncaughtExceptionHandler();
        Thread.setDefaultUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                String stack = Log.getStackTraceString(e);
                try {
                    File dir = getExternalFilesDir(null);
                    if (dir != null) {
                        File f = new File(dir, CRASH_FILE);
                        FileWriter w = new FileWriter(f, true);
                        w.append("\n==== CRASH ")
                          .append(new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.US).format(new Date()))
                          .append(" thread=").append(t.getName()).append(" ====\n");
                        w.append(stack);
                        String libs = com.samp.mobile.game.GTASA.loadDiagnostics;
                        if (libs != null) {
                            w.append("\n[LIB LOAD]\n").append(libs).append("\n");
                        }
                        w.append("[DEVICE] ").append(android.os.Build.MANUFACTURER)
                         .append(" ").append(android.os.Build.MODEL)
                         .append(" Android ").append(android.os.Build.VERSION.RELEASE)
                         .append(" API ").append(String.valueOf(android.os.Build.VERSION.SDK_INT))
                         .append("\n");
                        w.close();
                    }
                } catch (Throwable ignored) {
                }

                try {
                    Intent i = new Intent(getApplicationContext(), com.samp.mobile.launcher.ErrorActivity.class);
                    i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
                    i.putExtra("msg", stack.length() > 8000 ? stack.substring(0, 8000) : stack);
                    startActivity(i);
                } catch (Throwable ignored) {
                }

                if (prev != null) {
                    prev.uncaughtException(t, e);
                } else {
                    android.os.Process.killProcess(android.os.Process.myPid());
                }
            }
        });
    }
}
