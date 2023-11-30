package com.example.myapplication;

import android.app.Activity;
import android.app.NativeActivity;
import android.content.Context;
import android.content.res.Configuration;
import android.os.Bundle;
import android.text.Editable;
import android.text.InputType;
import android.text.method.DateKeyListener;
import android.text.method.KeyListener;
import android.text.method.TimeKeyListener;
import android.view.KeyEvent;
import android.text.TextWatcher;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.view.ViewTreeObserver;
import android.widget.Toast;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.TextView;
import android.text.method.DigitsKeyListener;
import android.graphics.Rect;

import java.util.concurrent.LinkedBlockingQueue;

public class MainActivity extends NativeActivity
implements TextWatcher, TextView.OnEditorActionListener
{
    // Used to load the 'C++' library on application startup.
    static {
        System.loadLibrary("myapplication");
    }

    protected View mView;
    private EditText mEditText;

    private native void OnKeyboardShown(boolean b);
    private native void OnScreenRotation(int deg);
    private native void OnInputCharacter(int ch);
    private native void OnSpecialKey(int code);

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);

         //create hidden EditText to access more soft keyboard options
        FrameLayout lay = new FrameLayout(this);
        setContentView(lay, new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));
        mEditText = new EditText(this);
        mEditText.setOnEditorActionListener(this);
        mEditText.addTextChangedListener(this);
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
        final int[] actionMap = {
                EditorInfo.IME_ACTION_NONE,
                EditorInfo.IME_ACTION_DONE, EditorInfo.IME_ACTION_GO,
                EditorInfo.IME_ACTION_NEXT, EditorInfo.IME_ACTION_PREVIOUS,
                EditorInfo.IME_ACTION_SEARCH, EditorInfo.IME_ACTION_SEND
        };
        final int type = _type;
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                InputMethodManager mgr = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
                //int sel1 = mEditText.getSelectionStart();
                //int sel2 = mEditText.getSelectionEnd();
                mEditText.setText("");
                mEditText.setImeOptions(actionMap[type >> 8]);
                mEditText.requestFocus();
                switch (type & 0xff) {
                    case 0:
                        mgr.hideSoftInputFromWindow(mEditText.getWindowToken(), 0);
                        break;
                    default: //ImeText
                        //TYPE_TEXT_VARIATION_VISIBLE_PASSWORD is needed otherwise NO_SUGGESTIONS will be ignored
                        mEditText.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS | InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD); //this resets selection
                        //mEditText.setSelection(sel1, sel2);
                        mgr.showSoftInput(mEditText, InputMethodManager.SHOW_IMPLICIT);
                        break;
                    case 2: //ImeNumber
                        mEditText.setInputType(InputType.TYPE_CLASS_NUMBER); //this resets selection
                        //mEditText.setSelection(sel1, sel2);
                        mgr.showSoftInput(mEditText, InputMethodManager.SHOW_IMPLICIT);
                        break;
                    case 3: //ImeDecimal
                        mEditText.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL); //this resets selection
                        mEditText.setRawInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL); //this resets selection
                        //mEditText.setSelection(sel1, sel2);
                        mgr.showSoftInput(mEditText, InputMethodManager.SHOW_IMPLICIT);
                        break;
                    case 4: //ImePhone
                        mEditText.setInputType(InputType.TYPE_CLASS_PHONE); //this resets selection
                        //mEditText.setSelection(sel1, sel2);
                        mgr.showSoftInput(mEditText, InputMethodManager.SHOW_IMPLICIT);
                        break;
                    case 5: //ImeEmail
                        mEditText.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS); //this resets selection
                        //mEditText.setSelection(sel1, sel2);
                        mgr.showSoftInput(mEditText, InputMethodManager.SHOW_IMPLICIT);
                        break;
                }
            }
        });
    }

    @Override
    public void beforeTextChanged(CharSequence var1, int var2, int var3, int var4) {
    }

    @Override
    public void afterTextChanged(Editable var1) {
    }

    @Override
    public void onTextChanged(CharSequence text, int start, int before, int count) {
        if (count < before)
            OnInputCharacter(0x8); //send backspace
        else if (count == before)
            ; //ignore, f.e. pressing @ in ImeEmail fires this 2x
        else if (count >= 1)
            OnInputCharacter((int)text.charAt(start + count - 1));
    }

    @Override
    public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
        OnSpecialKey(actionId + 256);
        return true;
    }

    /* Not called with IME_TEXT, not called on hw keyboard
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
    }*/

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
    public int getRotation() {
        int angle = 90 * getWindowManager().getDefaultDisplay().getRotation();
        return angle;
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
}