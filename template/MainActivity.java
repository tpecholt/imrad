package com.example.myapplication;

import android.app.Activity;
import android.app.NativeActivity;
import android.content.Context;
import android.content.res.Configuration;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.view.ViewTreeObserver;
import android.widget.Toast;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.text.method.DigitsKeyListener;
import android.graphics.Rect;

import java.util.concurrent.LinkedBlockingQueue;

public class MainActivity extends NativeActivity {

    // Used to load the 'C++' library on application startup.
    static {
        System.loadLibrary("myapplication");
    }

    protected View mView;
    private EditText mEditText;

    private native void OnKeyboardShown(boolean b);

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);

         //create hidden EditText to access more soft keyboard options
        FrameLayout lay = new FrameLayout(this);
        setContentView(lay, new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));
        mEditText = new EditText(this);
        mEditText.setLayoutParams(new FrameLayout.LayoutParams(1, 1));
        lay.addView(mEditText);
        mView = new View(this);
        mView.setLayoutParams(new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));
        lay.addView(mView);

        //install observer to monitor if keyboard is shown
        View mRootView = getWindow().getDecorView().findViewById(android.R.id.content);
        mRootView.getViewTreeObserver().addOnGlobalLayoutListener(
                new ViewTreeObserver.OnGlobalLayoutListener() {
                    public void onGlobalLayout() {
                        View view = getWindow().getDecorView();
                        int screenHeight = view.getRootView().getHeight();
                        Rect r = new Rect();
                        view.getWindowVisibleDisplayFrame(r);
                        double hkbd = 2.54 * (double)(screenHeight - r.bottom) / getDpi();
                        boolean kbdShown = hkbd > 3; //kbd height more than 3cm
                        OnKeyboardShown(kbdShown);
                    }
                });
    }

    public void showSoftInput(int _type) {
        final int type = _type;
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                InputMethodManager mgr = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
                mEditText.requestFocus();
                switch (type) {
                    case 0:
                        mgr.hideSoftInputFromWindow(mEditText.getWindowToken(), 0);
                        break;
                    case 1: //all
                        mEditText.setKeyListener(null);
                        mgr.showSoftInput(mEditText/*this.getWindow().getDecorView()*/, InputMethodManager.SHOW_IMPLICIT);
                        break;
                    case 2: //number
                        mEditText.setKeyListener(new DigitsKeyListener(false, false));
                        mgr.showSoftInput(mEditText/*this.getWindow().getDecorView()*/, InputMethodManager.SHOW_IMPLICIT);
                        break;
                    case 3: //decimal
                        mEditText.setKeyListener(new DigitsKeyListener(false, true));
                        mgr.showSoftInput(mEditText/*this.getWindow().getDecorView()*/, InputMethodManager.SHOW_IMPLICIT);
                        break;
                }
            }
        });
    }

    private LinkedBlockingQueue<Integer> unicodeQueue = new LinkedBlockingQueue<Integer>();

    @Override
    public boolean dispatchKeyEvent(KeyEvent ev) {
        if (ev.getAction() == KeyEvent.ACTION_DOWN) {
            unicodeQueue.offer(ev.getUnicodeChar(ev.getMetaState()));
        }
        else if (ev.getAction() == KeyEvent.ACTION_MULTIPLE) {
            for (int i = 0; i < ev.getCharacters().length(); ++i)
                unicodeQueue.offer(ev.getCharacters().codePointAt(i));
        }
        return super.dispatchKeyEvent(ev);
    }

    // JNI calls --------------------------------------------------

    public int getDpi() {
        Configuration cfg = getResources().getConfiguration();
        return cfg.densityDpi;
    }
    public void showMessage(String _msg) {
        final String msg = _msg;
        final Activity act = this;
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(act, msg, Toast.LENGTH_SHORT);
            }
        });
    }

    public int pollUnicodeChar() {
        try {
            return unicodeQueue.poll();
        } catch (NullPointerException e) {
            return 0;
        }
    }
}