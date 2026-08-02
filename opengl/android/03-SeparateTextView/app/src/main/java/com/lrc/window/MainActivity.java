package com.lrc.window;
import android.os.Bundle;
import android.graphics.Color;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        MyTextView myTextView = new MyTextView(this);
        getWindow().getDecorView().setBackgroundColor(Color.BLACK);
        
        setContentView(myTextView);
    }
}
