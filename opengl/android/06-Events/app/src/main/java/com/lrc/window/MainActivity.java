package com.lrc.window;
import android.os.Bundle;
import android.graphics.Color;
import android.content.pm.ActivityInfo;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;

public class MainActivity extends AppCompatActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // force landscape orientation
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);

        // FullScreen
        // hide action bar
        getSupportActionBar().hide();
        // tell android system to make your window expand edge to edge
        WindowCompat.setDecorFitsSystemWindows(getWindow(), false);
        // get window insets controller
        WindowInsetsControllerCompat windowInsetsControllerCompat = WindowCompat.getInsetsController(getWindow(), getWindow().getDecorView());
        // tell controller to hide remaining bar and insets
        windowInsetsControllerCompat.hide(WindowInsetsCompat.Type.systemBars() | WindowInsetsCompat.Type.ime());


        MyTextView myTextView = new MyTextView(this);
        getWindow().getDecorView().setBackgroundColor(Color.BLACK);
        
        setContentView(myTextView);
    }
}
