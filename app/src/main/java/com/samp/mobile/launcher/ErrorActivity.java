package com.samp.mobile.launcher;

import android.app.Activity;
import android.graphics.Color;
import android.graphics.Typeface;
import android.os.Bundle;
import android.view.Gravity;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

public class ErrorActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(32, 48, 32, 32);
        root.setBackgroundColor(Color.parseColor("#1B1B1F"));

        TextView title = new TextView(this);
        title.setText("Terjadi error — kirim screenshot ini");
        title.setTextColor(Color.parseColor("#FF8A80"));
        title.setTextSize(18);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        root.addView(title);

        ScrollView sv = new ScrollView(this);
        sv.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f));

        TextView tv = new TextView(this);
        tv.setTextIsSelectable(true);
        tv.setTypeface(Typeface.MONOSPACE);
        tv.setTextSize(12);
        tv.setTextColor(Color.WHITE);

        StringBuilder sb = new StringBuilder();
        String msg = getIntent().getStringExtra("msg");
        if (msg != null && !msg.isEmpty()) {
            sb.append("--- CRASH ---\n").append(msg).append("\n");
        }
        String libs = com.samp.mobile.game.GTASA.loadDiagnostics;
        if (libs != null && !libs.isEmpty()) {
            sb.append("\n--- LIB LOAD ---\n").append(libs).append("\n");
        }
        sb.append("\n--- DEVICE ---\n")
          .append("Model: ").append(android.os.Build.MANUFACTURER).append(" ")
          .append(android.os.Build.MODEL).append("\n")
          .append("Android: ").append(android.os.Build.VERSION.RELEASE)
          .append(" (API ").append(android.os.Build.VERSION.SDK_INT).append(")\n");

        tv.setText(sb.toString());
        sv.addView(tv);
        root.addView(sv);

        Button btn = new Button(this);
        btn.setText("Tutup aplikasi");
        btn.setOnClickListener(v -> {
            android.os.Process.killProcess(android.os.Process.myPid());
        });
        root.addView(btn);

        setContentView(root);
    }
}
