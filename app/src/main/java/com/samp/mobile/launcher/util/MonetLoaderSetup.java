package com.samp.mobile.launcher.util;

import android.content.Context;
import android.os.Environment;

import java.io.File;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class MonetLoaderSetup
{
    private static final String COMPAT_PROFILE_JSON =
        "{\n" +
        "  \"gtasa_name\": \"libGTASA.so\",\n" +
        "  \"profile_name\": \"SA-MP 0.3.7\",\n" +
        "  \"compat_scripts\": [],\n" +
        "  \"samp_name\": \"libsamp.so\",\n" +
        "  \"receiveignorerpc_pattern\": \"F0B503AF2DE900????B004460068C16A20468847\",\n" +
        "  \"cnetgame_ctor_pattern\": \"F0B503AF2DE9000788B00D46????9146????0446002079447A44\",\n" +
        "  \"rakclientinterface_netgame_offset\": 528,\n" +
        "  \"use_samp_touch_workaround\": true,\n" +
        "  \"nveventinsertnewest_offset\": 2606320\n" +
        "}";

    public static File getMonetLoaderDir(Context context)
    {
        File[] mediaDirs = context.getExternalMediaDirs();
        if(mediaDirs != null && mediaDirs.length > 0)
            return new File(mediaDirs[0], "monetloader");

        File fallback = new File(Environment.getExternalStorageDirectory(),
            "Android/media/" + context.getPackageName() + "/monetloader");
        fallback.mkdirs();
        return fallback;
    }

    public static boolean ensureInstalled(Context context)
    {
        try
        {
            String version = context.getPackageManager()
                .getPackageInfo(context.getPackageName(), 0).versionName;
            File root = getMonetLoaderDir(context);
            File marker = new File(root, ".stdlib_version");

            if(marker.exists())
            {
                java.io.BufferedReader r = new java.io.BufferedReader(new java.io.FileReader(marker));
                String installed = r.readLine();
                r.close();
                if(version.equals(installed))
                    return true;
            }

            copyAssetDir(context, "monetloader", root);

            marker.getParentFile().mkdirs();
            FileWriter w = new FileWriter(marker);
            w.write(version);
            w.flush();
            w.close();
            return true;
        }
        catch(Exception e)
        {
            e.printStackTrace();
            return false;
        }
    }

    public static void writeCompatProfile(Context context)
    {
        try
        {
            File compatDir = new File(getMonetLoaderDir(context), "compat");
            if(!compatDir.exists() && !compatDir.mkdirs())
                return;

            File profile = new File(compatDir, "profile.json");
            if(profile.isDirectory())
                profile.delete();

            if(!profile.exists())
            {
                FileWriter writer = new FileWriter(profile);
                writer.append(COMPAT_PROFILE_JSON);
                writer.flush();
                writer.close();
            }
        }
        catch(Exception e)
        {
            e.printStackTrace();
        }
    }

    private static void copyAssetDir(Context context, String assetPath, File targetDir) throws IOException
    {
        if(!targetDir.exists() && !targetDir.mkdirs())
            throw new IOException("Cannot create dir: " + targetDir);

        String[] entries = context.getAssets().list(assetPath);
        if(entries == null || entries.length == 0)
        {
            copySingleAsset(context, assetPath, new File(targetDir, fileName(assetPath)));
            return;
        }

        for(String entry : entries)
        {
            String childAsset = assetPath + "/" + entry;
            File childTarget = new File(targetDir, entry);
            String[] check = context.getAssets().list(childAsset);
            if(check != null && check.length > 0)
                copyAssetDir(context, childAsset, childTarget);
            else
                copySingleAsset(context, childAsset, childTarget);
        }
    }

    private static void copySingleAsset(Context context, String assetPath, File target) throws IOException
    {
        target.getParentFile().mkdirs();
        InputStream in = null;
        OutputStream out = null;
        try
        {
            in = context.getAssets().open(assetPath);
            out = new FileOutputStream(target);
            byte[] buffer = new byte[8192];
            int read;
            while((read = in.read(buffer)) != -1)
                out.write(buffer, 0, read);
        }
        finally
        {
            if(in != null) in.close();
            if(out != null) out.close();
        }
    }

    private static String fileName(String path)
    {
        int idx = path.lastIndexOf('/');
        return idx >= 0 ? path.substring(idx + 1) : path;
    }
}
