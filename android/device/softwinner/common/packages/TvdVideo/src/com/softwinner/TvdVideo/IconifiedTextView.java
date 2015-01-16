package com.softwinner.TvdVideo;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

public class IconifiedTextView extends LinearLayout {
	// 一个文件包括文件名和图表
	// 采用一个垂直线性布局
	private TextView mText = null;
	private ImageView mIcon = null;

	public IconifiedTextView(Context context, int id, String text) {
		super(context);
		// 设置布局方式
		View view;
		LayoutInflater inflate = (LayoutInflater) context
				.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		view = inflate.inflate(R.layout.list_item, null);
		mIcon = (ImageView) view.findViewById(R.id.list_icon);
		mText = (TextView) view.findViewById(R.id.list_text);
		mIcon.setImageDrawable(context.getResources().getDrawable(id));
		mText.setText(text);
		mText.setTextColor(0xffffffff);
		addView(view);

		// this.setOrientation(HORIZONTAL);
		// mIcon = new ImageView(context);
		// //设置ImageView为文件的图标
		// mIcon.setImageDrawable(context.getResources().getDrawable(id));
		// //设置图标在该布局中的填充位置
		// mIcon.setPadding(8, 6, 6, 6);
		// //将ImageView即图表添加到该布局中
		// addView(mIcon, new LinearLayout.LayoutParams(
		// LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
		// //设置文件名、填充方式、字体大小
		// mText = new TextView(context);
		// mText.setText(text);
		// mText.setPadding(8, 0, 6, 0);
		// mText.setTextSize(20);
		// mText.setSingleLine(true);
		// //将文件名添加到布局中
		// addView(mText, new LinearLayout.LayoutParams(
		// LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));
	}

	// 设置文件名
	public void setText(String words) {
		mText.setText(words);
	}

	// 设置图标
	public void setIcon(Drawable bullet) {
		mIcon.setImageDrawable(bullet);
	}
}
