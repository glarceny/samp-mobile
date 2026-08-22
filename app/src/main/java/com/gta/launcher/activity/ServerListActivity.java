package com.gta.launcher.activity;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Bundle;
import android.text.InputType;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import com.gta.launcher.util.GameDataUtil;

import org.json.JSONArray;
import org.json.JSONObject;

public class ServerListActivity extends Activity {
    private LinearLayout listContainer;
    private JSONArray servers;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        ScrollView scroll = new ScrollView(this);
        scroll.setBackgroundColor(Color.parseColor("#14141A"));
        scroll.setFillViewport(true);

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        int pad = dp(20);
        root.setPadding(pad, pad * 2, pad, pad);
        scroll.addView(root);

        TextView title = new TextView(this);
        title.setText("DAFTAR SERVER");
        title.setTextColor(Color.WHITE);
        title.setTextSize(24);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        title.setGravity(Gravity.CENTER);
        root.addView(title);

        TextView sub = new TextView(this);
        sub.setText("Ketuk server untuk connect, tahan untuk hapus");
        sub.setTextColor(Color.parseColor("#8A8A94"));
        sub.setTextSize(12);
        sub.setGravity(Gravity.CENTER);
        root.addView(sub);

        View sp1 = new View(this);
        sp1.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, dp(16)));
        root.addView(sp1);

        listContainer = new LinearLayout(this);
        listContainer.setOrientation(LinearLayout.VERTICAL);
        root.addView(listContainer);

        Button addBtn = new Button(this);
        addBtn.setText("+ Tambah Server");
        addBtn.setTextColor(Color.WHITE);
        addBtn.setBackgroundTintList(android.content.res.ColorStateList.valueOf(Color.parseColor("#2E7D32")));
        root.addView(addBtn);
        addBtn.setOnClickListener(v -> showAddDialog());

        setContentView(scroll);
        loadServers();
    }

    private String prefsName() { return "stenly_servers"; }

    private void loadServers() {
        servers = new JSONArray();
        try {
            String raw = getSharedPreferences(prefsName(), MODE_PRIVATE).getString("list", null);
            if (raw != null) servers = new JSONArray(raw);
        } catch (Throwable ignored) {}
        if (servers.length() == 0) {
            try {
                JSONObject def = new JSONObject();
                def.put("name", "Server Pertama");
                def.put("host", "141.95.234.17:7777");
                servers.put(def);
                saveServers();
            } catch (Throwable ignored) {}
        }
        rebuild();
    }

    private void saveServers() {
        getSharedPreferences(prefsName(), MODE_PRIVATE)
                .edit().putString("list", servers.toString()).apply();
    }

    private void rebuild() {
        listContainer.removeAllViews();
        for (int i = 0; i < servers.length(); i++) {
            final int idx = i;
            try {
                JSONObject o = servers.getJSONObject(i);
                listContainer.addView(makeItem(o.optString("name", "?"),
                        o.optString("host", "0.0.0.0:7777"), idx));
            } catch (Throwable ignored) {}
        }
    }

    private View makeItem(String name, String host, int index) {
        LinearLayout item = new LinearLayout(this);
        item.setOrientation(LinearLayout.VERTICAL);
        item.setPadding(dp(16), dp(12), dp(16), dp(12));
        item.setBackgroundResource(android.R.drawable.dialog_holo_light_frame);
        item.setBackgroundColor(Color.parseColor("#1E1E26"));

        TextView n = new TextView(this);
        n.setText(name);
        n.setTextColor(Color.WHITE);
        n.setTextSize(17);
        n.setTypeface(Typeface.DEFAULT_BOLD);
        item.addView(n);

        TextView h = new TextView(this);
        h.setText(host);
        h.setTextColor(Color.parseColor("#7FBF7F"));
        h.setTextSize(13);
        item.addView(h);

        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        lp.topMargin = dp(10);
        item.setLayoutParams(lp);

        item.setOnClickListener(v -> confirmConnect(name, host));
        item.setOnLongClickListener(v -> {
            confirmDelete(index, name);
            return true;
        });
        return item;
    }

    private void showAddDialog() {
        LinearLayout box = new LinearLayout(this);
        box.setOrientation(LinearLayout.VERTICAL);
        int p = dp(20);
        box.setPadding(p, p / 2, p, 0);

        final EditText nameIn = new EditText(this);
        nameIn.setHint("Nama server");
        box.addView(nameIn);

        final EditText hostIn = new EditText(this);
        hostIn.setHint("IP:PORT (contoh 1.2.3.4:7777)");
        hostIn.setInputType(InputType.TYPE_CLASS_TEXT);
        box.addView(hostIn);

        new AlertDialog.Builder(this)
                .setTitle("Tambah Server")
                .setView(box)
                .setPositiveButton("Simpan", (d, w) -> {
                    String nm = nameIn.getText().toString().trim();
                    String hs = hostIn.getText().toString().trim();
                    if (nm.isEmpty() || !hs.contains(":")) {
                        Toast.makeText(this, "Nama & IP:PORT wajib diisi", Toast.LENGTH_SHORT).show();
                        return;
                    }
                    try {
                        JSONObject o = new JSONObject();
                        o.put("name", nm);
                        o.put("host", hs);
                        servers.put(o);
                        saveServers();
                        rebuild();
                    } catch (Throwable ignored) {}
                })
                .setNegativeButton("Batal", null)
                .show();
    }

    private void confirmDelete(int index, String name) {
        new AlertDialog.Builder(this)
                .setTitle("Hapus server?")
                .setMessage(name)
                .setPositiveButton("Hapus", (d, w) -> {
                    JSONArray arr = new JSONArray();
                    for (int i = 0; i < servers.length(); i++) {
                        if (i == index) continue;
                        try { arr.put(servers.get(i)); } catch (Throwable ignored) {}
                    }
                    servers = arr;
                    saveServers();
                    rebuild();
                })
                .setNegativeButton("Batal", null)
                .show();
    }

    private void confirmConnect(String name, String host) {
        if (!GameDataUtil.isDataReady()) {
            new AlertDialog.Builder(this)
                    .setTitle("Data game belum ada")
                    .setMessage("Untuk bermain, download cache game lalu extract ke penyimpanan internal.\n\nTetap tidak bisa main tanpa data.")
                    .setPositiveButton("Buka Link Download", (d, w) -> {
                        try {
                            startActivity(new android.content.Intent(
                                    android.content.Intent.ACTION_VIEW, Uri.parse(GameDataUtil.CACHE_URL)));
                        } catch (Throwable ignored) {}
                    })
                    .setNegativeButton("Tutup", null)
                    .show();
            return;
        }
        new AlertDialog.Builder(this)
                .setTitle("Connect")
                .setMessage(name + "\n" + host)
                .setPositiveButton("Masuk", (d, w) -> connect(host))
                .setNegativeButton("Batal", null)
                .show();
    }

    private void connect(String hostPort) {
        try {
            String hp = hostPort.trim();
            String host = hp;
            int port = 7777;
            int c = hp.lastIndexOf(':');
            if (c > 0) {
                host = hp.substring(0, c);
                port = Integer.parseInt(hp.substring(c + 1));
            }
            String nick = getSharedPreferences("stenly", MODE_PRIVATE)
                    .getString("nick", GameDataUtil.readNick("Stenly" + (100 + (int)(Math.random() * 900))));
            GameDataUtil.writeConnection(nick, host, port, "");
            Toast.makeText(this, "Menghubungkan sebagai " + nick, Toast.LENGTH_SHORT).show();
            startActivity(new Intent(this, com.gta.game.SAMP.class));
        } catch (Throwable t) {
            Toast.makeText(this, "Gagal: " + t.getMessage(), Toast.LENGTH_LONG).show();
        }
    }

    private int dp(int v) { return (int) (v * getResources().getDisplayMetrics().density); }
}
