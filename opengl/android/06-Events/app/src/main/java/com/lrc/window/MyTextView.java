package com.lrc.window;

import android.graphics.Color;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.GestureDetector;
import android.view.GestureDetector.OnGestureListener;
import android.view.GestureDetector.OnDoubleTapListener;
import android.content.Context;

import androidx.appcompat.widget.AppCompatTextView;

public class MyTextView extends AppCompatTextView implements OnGestureListener, OnDoubleTapListener {
    private GestureDetector gestureDetector;

    public MyTextView(Context context) {
        super(context);
        setTextColor(Color.rgb(0, 255, 0));
        setTextSize(60);
        setGravity(Gravity.CENTER);
        setText("Hello World!");

        gestureDetector = new GestureDetector(context, this, null, false);
        gestureDetector.setOnDoubleTapListener(this);
    }

    @Override
    public boolean onTouchEvent(MotionEvent e) {
        if (!gestureDetector.onTouchEvent(e)) {
            super.onTouchEvent(e);
        }
        return true;
    }
    @Override
    public boolean onDoubleTap(MotionEvent e) {
        setText("DoubleTap");
        return true;
    }
    @Override
    public boolean onSingleTapConfirmed(MotionEvent e) {
        setText("SingleTap");
        return true;
    }
    @Override
    public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
        setText("Scroll");
        return true;
    }
    @Override
    public boolean onDoubleTapEvent(MotionEvent e) { return true; }
    @Override
    public boolean onDown(MotionEvent e) { return true; }
    @Override
    public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) { return true; }
    @Override
    public void onLongPress(MotionEvent e) { setText("Long Press"); }
    @Override
    public boolean onSingleTapUp(MotionEvent e) { return true; }
    @Override
    public void onShowPress(MotionEvent e) {}
}
