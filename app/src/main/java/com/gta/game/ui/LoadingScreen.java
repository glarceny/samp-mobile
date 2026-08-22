package com.gta.game.ui;

import android.app.Activity;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.ProgressBar;

import androidx.constraintlayout.widget.ConstraintLayout;

import com.gta.game.R;

public class LoadingScreen {

    private Activity activity;
    private ConstraintLayout mainLayout;
    private ProgressBar progressBar;

    private Handler handler;
    private Runnable progressRunnable;
    private int progress = 0;
    private static final int TOTAL_DURATION = 10000;
    private static final int UPDATE_INTERVAL = 40;

    public LoadingScreen(Activity activity) {
        this.activity = activity;

        mainLayout = (ConstraintLayout) activity.getLayoutInflater()
                .inflate(R.layout.loadingscreen, null);
        activity.addContentView(mainLayout,
                new ConstraintLayout.LayoutParams(
                        ConstraintLayout.LayoutParams.MATCH_PARENT,
                        ConstraintLayout.LayoutParams.MATCH_PARENT));

        initializeViews();
        startProgressAnimation();
    }

    private void initializeViews() {
        progressBar = mainLayout.findViewById(R.id.progressBar2);
        handler = new Handler(Looper.getMainLooper());
    }

    private void startProgressAnimation() {
        progressRunnable = new Runnable() {
            @Override
            public void run() {
                if (progress < 100) {
                    progress++;
                    progressBar.setProgress(progress);
                    handler.postDelayed(this, UPDATE_INTERVAL);
                } else {
                    onLoadingComplete();
                }
            }
        };

        handler.postDelayed(progressRunnable, 500);
    }

    private void onLoadingComplete() {
        Animation fadeOut = AnimationUtils.loadAnimation(activity, R.anim.fade_out);
        mainLayout.startAnimation(fadeOut);

        fadeOut.setAnimationListener(new Animation.AnimationListener() {
            @Override
            public void onAnimationStart(Animation animation) { }

            @Override
            public void onAnimationEnd(Animation animation) {
                hide();
            }

            @Override
            public void onAnimationRepeat(Animation animation) { }
        });
    }

    public void hide() {
        if (handler != null && progressRunnable != null) {
            handler.removeCallbacks(progressRunnable);
        }

        if (mainLayout != null) {
            mainLayout.setVisibility(View.GONE);
        }
    }

    public void show() {
        if (mainLayout != null) {
            mainLayout.setVisibility(View.VISIBLE);
            progress = 0;
            progressBar.setProgress(0);
            startProgressAnimation();
        }
    }

    public void destroy() {
        hide();
        handler = null;
        progressRunnable = null;
    }
}