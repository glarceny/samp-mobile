package com.gta.launcher.activity;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.InputType;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.gta.launcher.util.GameDataUtil;
import com.gta.launcher.util.SampQuery;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class ServerListActivity extends Activity {
    private static final int REFRESH_MS = 6000;

    private ListView listServers;
    private TextView tvSubtitle, emptyView;
    private JSONArray servers = new JSONArray();
    private final Map<String, SampQuery.Info> infoCache = new HashMap<>();
    private ServerAdapter adapter;
    private final ExecutorService pool = Executors.newSingleThreadExecutor();
    private final Handler ui = new Handler(Looper.getMainLooper());
    private Runnable autoRefresh;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_server_list);

        listServers = findViewById(R.id.listServers);
        tvSubtitle = findViewById(R.id.tvSubtitle);
        findViewById(R.id.btnAddServer).setOnClickListener(v -> showAddDialog());

        emptyView = new TextView(this);
        emptyView.setText("Belum ada server.\nTekan + Server untuk menambah.");
        emptyView.setTextColor(Color.parseColor("#6B7280"));
        emptyView.setTextSize(14);
        emptyView.setGravity(android.view.Gravity.CENTER);
        emptyView.setPadding(0, dp(80), 0, 0);
        listServers.setEmptyView(emptyView);

        adapter = new ServerAdapter();
        listServers.setAdapter(adapter);
        listServers.setOnItemClickListener((p, v, pos, id) -> {
            try {
                JSONObject o = servers.getJSONObject(pos);
                showConnectSheet(o.optString("name", "?"), o.optString("host", ""));
            } catch (Throwable ignored) {}
        });
        listServers.setOnItemLongClickListener((p, v, pos, id) -> {
            try {
                JSONObject o = servers.getJSONObject(pos);
                confirmDelete(pos, o.optString("name", "?"));
            } catch (Throwable ignored) {}
            return true;
        });

        loadServers();
        refreshAll();
    }

    @Override
    protected void onResume() {
        super.onResume();
        autoRefresh = new Runnable() {
            @Override public void run() {
                refreshAll();
                ui.postDelayed(this, REFRESH_MS);
            }
        };
        ui.postDelayed(autoRefresh, REFRESH_MS);
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (autoRefresh != null) ui.removeCallbacks(autoRefresh);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        pool.shutdownNow();
    }

    private void loadServers() {
        try {
            String raw = getSharedPreferences("stenly_servers", MODE_PRIVATE).getString("list", null);
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
        adapter.notifyDataSetChanged();
        updateSubtitle();
    }

    private void saveServers() {
        getSharedPreferences("stenly_servers", MODE_PRIVATE)
                .edit().putString("list", servers.toString()).apply();
    }

    private void updateSubtitle() {
        int online = 0;
        for (SampQuery.Info i : infoCache.values()) if (i.online) online++;
        tvSubtitle.setText("Stenly Client • " + online + "/" + servers.length() + " online");
    }

    private void refreshAll() {
        for (int i = 0; i < servers.length(); i++) {
            final String host;
            try { host = servers.getJSONObject(i).optString("host", ""); } catch (Throwable t) { continue; }
            pool.execute(() -> {
                String[] hp = splitHostPort(host);
                SampQuery.Info inf = SampQuery.queryInfo(hp[0], Integer.parseInt(hp[1]), 1200);
                infoCache.put(host, inf);
                ui.post(() -> {
                    adapter.notifyDataSetChanged();
                    updateSubtitle();
                });
            });
        }
    }

    private static String[] splitHostPort(String hostPort) {
        String h = hostPort.trim();
        String host = h;
        int port = 7777;
        int c = h.lastIndexOf(':');
        if (c > 0) {
            host = h.substring(0, c);
            try { port = Integer.parseInt(h.substring(c + 1).trim()); } catch (Throwable ignored) {}
        }
        return new String[]{host, String.valueOf(port)};
    }

    private class ServerAdapter extends BaseAdapter {
        @Override public int getCount() { return servers.length(); }
        @Override public Object getItem(int position) { return position; }
        @Override public long getItemId(int position) { return position; }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            View v = convertView;
            if (v == null) {
                v = LayoutInflater.from(ServerListActivity.this)
                        .inflate(R.layout.item_server, parent, false);
            }
            try {
                JSONObject o = servers.getJSONObject(position);
                String name = o.optString("name", "?");
                String host = o.optString("host", "");
                String[] hp = splitHostPort(host);

                ((TextView) v.findViewById(R.id.tvName)).setText(name);
                ((TextView) v.findViewById(R.id.tvHost)).setText(host);

                TextView tvMode = v.findViewById(R.id.tvMode);
                TextView tvPlayers = v.findViewById(R.id.tvPlayers);
                TextView tvPing = v.findViewById(R.id.tvPing);
                View dot = v.findViewById(R.id.dotPing);

                SampQuery.Info inf = infoCache.get(host);
                if (inf == null) {
                    tvMode.setText("memuat…");
                    tvPlayers.setText("—");
                    tvPing.setText("—");
                    setDot(dot, "#666666");
                } else if (!inf.online) {
                    tvMode.setText("offline");
                    tvPlayers.setText("—");
                    tvPing.setText("—");
                    setDot(dot, "#E57373");
                } else {
                    String mode = inf.gamemode.isEmpty() ? "" : inf.gamemode;
                    if (!inf.language.isEmpty()) mode += (mode.isEmpty() ? "" : " • ") + inf.language;
                    tvMode.setText(mode.isEmpty() ? "online" : mode);
                    tvPlayers.setText(inf.players + "/" + inf.maxPlayers);
                    tvPing.setText(inf.pingMs < 0 ? "—" : inf.pingMs + " ms");
                    setDot(dot, inf.pingMs < 80 ? "#81C784" : (inf.pingMs < 200 ? "#FFD54F" : "#E57373"));
                    if (inf.passworded) {
                        tvMode.setText("🔒 " + tvMode.getText());
                    }
                }
            } catch (Throwable ignored) {}
            return v;
        }

        private void setDot(View dot, String color) {
            GradientDrawable d = new GradientDrawable();
            d.setShape(GradientDrawable.OVAL);
            d.setColor(Color.parseColor(color));
            dot.setBackground(d);
        }
    }

    private void showAddDialog() {
        LinearLayout box = new LinearLayout(this);
        box.setOrientation(LinearLayout.VERTICAL);
        int p = dp(22);
        box.setPadding(p, p / 2, p, 0);

        final EditText nameIn = styledInput();
        nameIn.setHint("Nama server");
        box.addView(nameIn);

        final EditText hostIn = styledInput();
        hostIn.setHint("IP:PORT — contoh: 1.2.3.4:7777");
        box.addView(hostIn);

        new AlertDialog.Builder(this)
                .setTitle("Tambah Server")
                .setView(box)
                .setPositiveButton("Simpan", (d, w) -> {
                    String nm = nameIn.getText().toString().trim();
                    String hs = hostIn.getText().toString().trim();
                    if (nm.isEmpty() || !hs.matches(".+:\\d+")) {
                        Toast.makeText(this, "Isi nama & format IP:PORT", Toast.LENGTH_SHORT).show();
                        return;
                    }
                    try {
                        JSONObject o = new JSONObject();
                        o.put("name", nm);
                        o.put("host", hs);
                        servers.put(o);
                        saveServers();
                        adapter.notifyDataSetChanged();
                        updateSubtitle();
                        refreshAll();
                    } catch (Throwable ignored) {}
                })
                .setNegativeButton("Batal", null)
                .show();
    }

    private EditText styledInput() {
        EditText e = new EditText(this);
        e.setTextColor(Color.WHITE);
        e.setHintTextColor(Color.parseColor("#6B7280"));
        e.setBackgroundResource(R.drawable.bg_input);
        e.setInputType(InputType.TYPE_CLASS_TEXT);
        e.setSingleLine(true);
        int p8 = dp(14), v8 = dp(10);
        e.setPadding(p8, v8, p8, v8);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        lp.topMargin = dp(10);
        e.setLayoutParams(lp);
        return e;
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
                    adapter.notifyDataSetChanged();
                    updateSubtitle();
                })
                .setNegativeButton("Batal", null)
                .show();
    }

    private void showConnectSheet(String name, String host) {
        BottomSheetDialog sheet = new BottomSheetDialog(this);
        View v = LayoutInflater.from(this).inflate(R.layout.sheet_connect, null, false);

        TextView tvName = v.findViewById(R.id.tvSheetName);
        TextView tvHost = v.findViewById(R.id.tvSheetHost);
        TextView tvInfo = v.findViewById(R.id.tvSheetInfo);
        TextView tvNote = v.findViewById(R.id.tvDataNote);
        EditText inNick = v.findViewById(R.id.inNick);
        EditText inPass = v.findViewById(R.id.inPass);
        Button btnPlay = v.findViewById(R.id.btnPlay);
        LinearLayout playerList = v.findViewById(R.id.playerList);

        String[] hp = splitHostPort(host);
        String ip = hp[0];
        int port = Integer.parseInt(hp[1]);

        tvName.setText(name);
        tvHost.setText(host);

        String savedNick = getSharedPreferences("stenly", MODE_PRIVATE).getString("nick",
                GameDataUtil.readNick("Stenly" + (100 + (int) (Math.random() * 900))));
        inNick.setText(savedNick);
        inNick.setSelection(savedNick.length());

        boolean dataOk = GameDataUtil.isDataReady();
        if (!dataOk) {
            btnPlay.setBackgroundTintList(android.content.res.ColorStateList.valueOf(Color.parseColor("#5A5A5A")));
            tvNote.setText("✖ Data game belum ada — tidak bisa MAIN. Ketuk di sini untuk buka link download.");
            tvNote.setOnClickListener(x -> startActivity(new Intent(Intent.ACTION_VIEW,
                    Uri.parse(GameDataUtil.CACHE_URL))));
        } else {
            tvNote.setTextColor(Color.parseColor("#81C784"));
            tvNote.setText("✔ Data game terpasang");
        }

        btnPlay.setOnClickListener(x -> {
            String nick = inNick.getText().toString().trim();
            if (nick.isEmpty()) {
                Toast.makeText(this, "Isi Nama_Player dulu", Toast.LENGTH_SHORT).show();
                return;
            }
            if (!GameDataUtil.isDataReady()) {
                Toast.makeText(this, "Data game belum ada — tidak bisa main", Toast.LENGTH_LONG).show();
                return;
            }
            getSharedPreferences("stenly", MODE_PRIVATE).edit().putString("nick", nick).apply();
            try {
                GameDataUtil.writeConnection(nick, ip, port, inPass.getText().toString());
                Toast.makeText(this, "Menghubungkan ke " + name + "…", Toast.LENGTH_SHORT).show();
                sheet.dismiss();
                startActivity(new Intent(this, com.gta.game.SAMP.class));
            } catch (Throwable t) {
                Toast.makeText(this, "Gagal: " + t.getMessage(), Toast.LENGTH_LONG).show();
            }
        });

        SampQuery.Info cached = infoCache.get(host);
        if (cached != null && cached.online) {
            tvInfo.setText(cached.players + "/" + cached.maxPlayers + " pemain • "
                    + (cached.pingMs >= 0 ? cached.pingMs + " ms" : "?")
                    + (cached.passworded ? " • berpassword" : ""));
        } else {
            tvInfo.setText("memeriksa server…");
        }

        pool.execute(() -> {
            List<SampQuery.Player> players = SampQuery.queryPlayers(ip, port, 1500);
            ui.post(() -> {
                playerList.removeAllViews();
                if (players.isEmpty()) {
                    TextView t = new TextView(this);
                    t.setText(players.isEmpty() && (cached == null || !cached.online)
                            ? "Tidak dapat mengambil daftar (server offline?)"
                            : "Tidak ada pemain online");
                    t.setTextColor(Color.parseColor("#6B7280"));
                    t.setTextSize(12);
                    t.setPadding(0, dp(6), 0, 0);
                    playerList.addView(t);
                    return;
                }
                for (SampQuery.Player pl : players) {
                    TextView row = new TextView(this);
                    row.setText(pl.name + "   •   skor " + pl.score + "   •   " + pl.ping + " ms");
                    row.setTextColor(Color.parseColor("#C9CEDA"));
                    row.setTextSize(13);
                    row.setPadding(0, dp(4), 0, dp(4));
                    playerList.addView(row);
                }
            });
        });

        sheet.setContentView(v);
        sheet.show();
    }

    private int dp(int v) { return (int) (v * getResources().getDisplayMetrics().density); }
}
