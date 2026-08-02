package com.lrc.window;

import android.graphics.Color;
import android.view.Gravity;
import android.content.Context;

import androidx.appcompat.widget.AppCompatTextView;

public class MyTextView extends AppCompatTextView {
    public MyTextView(Context context) {
        super(context);
        setTextColor(Color.rgb(0, 255, 9));
        setTextSize(60);
        setGravity(Gravity.CENTER);
        setText("Hello World!");
    }
}
