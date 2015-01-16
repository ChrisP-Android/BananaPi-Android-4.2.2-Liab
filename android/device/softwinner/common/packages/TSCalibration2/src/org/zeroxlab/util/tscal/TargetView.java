/* Copyright (C) 2010 0xlab.org
 * Authored by: Kan-Ru Chen <kanru@0xlab.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.zeroxlab.util.tscal;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.View;

public class TargetView extends View {

    public TargetView(Context context, AttributeSet attrs)
    {
        super(context);
    }

    @Override
    protected void onMeasure (int widthMeasureSpec, int heightMeasureSpec)
    {
        setMeasuredDimension(50, 50);
    }

    @Override
    protected void onDraw(Canvas c) {
        Paint white = new Paint(Paint.ANTI_ALIAS_FLAG);
        Paint red = new Paint(Paint.ANTI_ALIAS_FLAG);
        white.setColor(Color.WHITE);
        red.setColor(Color.RED);

        c.drawColor(Color.BLACK);
        c.translate(25, 25);
        c.drawCircle(0, 0, 25, red);
        c.drawCircle(0, 0, 21, white);
        c.drawCircle(0, 0, 17, red);
        c.drawCircle(0, 0, 13, white);
        c.drawCircle(0, 0, 9, red);
        c.drawCircle(0, 0, 5, white);
        c.drawCircle(0, 0, 1, red);
    }
}
