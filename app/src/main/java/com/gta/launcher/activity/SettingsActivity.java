package com.gta.launcher.activity;

import android.app.Activity;
import android.app.AlertDialog;
import android.graphics.Color;
import android.graphics.Typeface;
import android.os.Bundle;
import android.view.Gravity;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import com.gta.launcher.util.GameDataUtil;

public class SettingsActivity extends Activity {
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
        title.setText("PENGATURAN");
        title.setTextColor(Color.WHITE);
        title.setTextSize(24);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        title.setGravity(Gravity.CENTER);
        root.addView(title);

        View sp = new View(this);
        sp.setLayoutParams(new LinearLayout.LayoutParams(1, dp(24)));
        root.addView(sp);

        TextView lbl = new TextView(this);
        lbl.setText("Nama karakter (nickname)");
        lbl.setTextColor(Color.parseColor("#8A8A94"));
        lbl.setTextSize(13);
        root.addView(lbl);

        EditText nick = new EditText(this);
        nick.setTextColor(Color.WHITE);
        nick.setText(getSharedPreferences("stenly", MODE_PRIVATE)
                .getString("nick", GameDataUtil.readNick("Player")));
        nick.setSelection(nick.getText().length());
        root.addView(nick);

        Button save = new Button(this);
        save.setText("Simpan");
        save.setTextColor(Color.WHITE);
        save.setBackgroundTintList(android.content.res.ColorStateList.valueOf(Color.parseColor("#2E7D32")));
        root.addView(save);
        save.setOnClickListener(v -> {
            String n = nick.getText().toString().trim();
            if (n.isEmpty()) {
                Toast.makeText(this, "Nickname tidak boleh kosong", Toast.LENGTH_SHORT).show();
                return;
            }
            getSharedPreferences("stenly", MODE_PRIVATE)
                    .edit().putString("nick", n).apply();
            try {
                GameDataUtil.writeConnection(n,
                        GameDataUtil.SETTINGS_INI != null ? currentHost() : "141.95.234.17",
                        currentPort(), "");
                Toast.makeText(this, "Tersimpan", Toast.LENGTH_SHORT).show();
            } catch (Throwable t) {
                Toast.makeText(this, "Nick disimpan (settings.ini menunggu data)", Toast.LENGTH_SHORT).show();
            }
            finish();
        });

        View sp2 = new View(this);
        sp2.setLayoutParams(new LinearLayout.LayoutParams(1, dp(28)));
        root.addView(sp2);

        TextView about = new TextView(this);
        about.setText("Stenly Client v0.9.1\nBerbasis SA-MP Mobile 2.11 (open source)\n\nStatus data: " +
                (GameDataUtil.isDataReady() ? "TERPASANG" : "BELUM ADA") +
                "\nFolder data: /storage/emulated/0/VICE");
        about.setTextColor(Color.parseColor("#8A8A94"));
        about.setTextSize(13);
        about.setGravity(Gravity.CENTER);
        root.addView(about);

        setContentView(scroll);
    }

    private String currentHost() {
        try { return GameDataUtil.readHost("141.95.234.17"); } catch (Throwable t) { return "141.95.234.17"; }
    }

    private int currentPort() {
        try { return GameDataUtil.readPort(7777); } catch (Throwable t) { return 7777; }
    }

    private int dp(int v) { return (int) (v * getResources().getDisplayMetrics().density); }
}
