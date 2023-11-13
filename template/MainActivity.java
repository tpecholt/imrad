package com.example.myapplication;

import android.app.Activity;
import android.app.NativeActivity;
import android.content.Context;
import android.content.res.Configuration;
import android.os.Bundle;
import android.text.Editable;
import android.text.InputType;
import android.text.method.DateKeyListener;
import android.text.method.TimeKeyListener;
import android.view.KeyEvent;
import android.text.TextWatcher;
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
    private TextWatcher mTextWatcher;

    private native void OnKeyboardShown(boolean b);
    private native void OnScreenRotation(int deg);

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

    //@param type - see ImRad::IMEType
    public void showSoftInput(int _type) {
        final int type = _type;
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                InputMethodManager mgr = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
                mEditText.requestFocus();
                mEditText.setText("");
                mEditText.removeTextChangedListener(mTextWatcher);
                switch (type) {
                    case -1:
                        mgr.hideSoftInputFromWindow(mEditText.getWindowToken(), 0);
                        break;
                    case 0: //all
                        mEditText.setKeyListener(null);
                        mgr.showSoftInput(mEditText, InputMethodManager.SHOW_IMPLICIT);
                        break;
                    case 1: //number
                        mEditText.setKeyListener(new DigitsKeyListener(false, false));
                        mgr.showSoftInput(mEditText, InputMethodManager.SHOW_IMPLICIT);
                        break;
                    case 2:
                        mEditText.setKeyListener(new DigitsKeyListener(true, true));
                        //dispatchKeyEvent is not called on special keyboard characters so do it from the listener
                        mTextWatcher = new TextWatcher() {
                            public void beforeTextChanged(CharSequence var1, int var2, int var3, int var4) {
                            }

                            public void afterTextChanged(Editable var1) {
                                //TODO: better
                                int i = var1.toString().indexOf('.');
                                if (i >= 0) {
                                    unicodeQueue.offer((int)'.');
                                    //delete dot so that pressing it again is not blocked
                                    //such as when focus changed to another widget etc.
                                    var1.delete(i, i + 1);
                                }
                            }

                            public void onTextChanged(CharSequence text, int start, int var3, int count) {
                                /*if (count == 1 && text.charAt(start) == '.')
                                    unicodeQueue.offer((int) '.');*/
                            }
                        };
                        mEditText.addTextChangedListener(mTextWatcher);
                        //mEditText.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
                        mgr.showSoftInput(mEditText, InputMethodManager.SHOW_IMPLICIT);
                        break;
                    case 3: //date
                        mEditText.setKeyListener(new DateKeyListener());
                        mgr.showSoftInput(mEditText, InputMethodManager.SHOW_IMPLICIT);
                        break;
                    case 4: //time
                        mEditText.setKeyListener(new TimeKeyListener());
                        mgr.showSoftInput(mEditText, InputMethodManager.SHOW_IMPLICIT);
                        break;
                }
            }
        });
    }

    private LinkedBlockingQueue<Integer> unicodeQueue = new LinkedBlockingQueue<Integer>();

    @Override
    public boolean dispatchKeyEvent(KeyEvent ev) {
        if (ev.getAction() == KeyEvent.ACTION_DOWN) {
            int ch = ev.getUnicodeChar(ev.getMetaState());
            if (ch >= 0x20) //control characters handled elsewhere
                unicodeQueue.offer(ch);
        }
        else if (ev.getAction() == KeyEvent.ACTION_MULTIPLE) {
            for (int i = 0; i < ev.getCharacters().length(); ++i) {
                int ch = ev.getCharacters().codePointAt(i);
                if (ch >= 0x20) //control characters handled elsewhere
                    unicodeQueue.offer(ch);
            }
        }
        return super.dispatchKeyEvent(ev);
    }

    @Override
    public void onConfigurationChanged(Configuration cfg) {
        super.onConfigurationChanged(cfg);
        int angle = 90 * getWindowManager().getDefaultDisplay().getRotation();
        OnScreenRotation(angle);
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