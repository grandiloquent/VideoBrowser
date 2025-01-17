package euphoria.psycho.downloader;

import android.app.Notification.Builder;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.os.Environment;
import android.view.View;

import java.io.File;

import androidx.annotation.RequiresApi;
import euphoria.psycho.explorer.R;
import euphoria.psycho.explorer.VideoListActivity;
import euphoria.psycho.share.FileShare;
import euphoria.psycho.share.KeyShare;

public class DownloaderHelper {

    private static final String TAG = "VideoHelper";

    public static boolean checkIfExistsRunningTask(RequestQueue queue) {
        return queue.getCurrentRequests()
                .stream()
                .anyMatch(r -> r.getDownloaderTask().Status != 7 && r.getDownloaderTask().Status > -1);
    }

    public static boolean checkTask(Context context, RequestQueue q, String fileName) {
        if (q.taskExists(fileName)) {
            context.sendBroadcast(new Intent(DownloaderActivity.ACTION_REFRESH));
            return true;
        }
        return false;
    }


    @RequiresApi(api = VERSION_CODES.O)
    public static void createNotificationChannel(Context context, NotificationManager manager) {
        final NotificationChannel notificationChannel = new NotificationChannel(
                DownloaderService.DOWNLOAD_CHANNEL,
                context.getString(R.string.channel_download_videos),
                NotificationManager.IMPORTANCE_LOW);
        manager.createNotificationChannel(notificationChannel);
    }

    public static void deleteVideoDirectory(Context context) {
        File directory =
                //FileShare.isHasSD() ? new File(FileShare.getExternalStoragePath(this), "Videos") :
                context.getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS);
        FileShare.recursivelyDeleteFile(directory);
        if (!directory.exists()) {
            directory.mkdirs();
        }
    }

    public static Builder getBuilder(Context context) {
        Builder builder;
        if (VERSION.SDK_INT >= VERSION_CODES.O) {
            builder = new Builder(context,
                    DownloaderService.DOWNLOAD_CHANNEL);
        } else {
            builder = new Builder(context);
        }
        builder.setSmallIcon(android.R.drawable.stat_sys_download)
                .setLocalOnly(true)
                .setColor(context.getColor(android.R.color.primary_text_dark))
                .setOngoing(true)
                .setContentIntent(PendingIntent.getActivity(
                        context,
                        0,
                        getVideoActivityIntent(context),
                        PendingIntent.FLAG_UPDATE_CURRENT
                ));
        return builder;
    }

    public static String[] getInfos(String uri) {
        String m3u8String = null;
        String fileName = null;
        try {
            m3u8String = "";//HLSUtils.getString(uri);
        } catch (Exception ignored) {
        }
        if (m3u8String == null) {
            return null;
        }
        try {
            fileName = KeyShare.toHex(KeyShare.md5encode(m3u8String));
        } catch (Exception e) {
            e.printStackTrace();
        }
        if (fileName == null) {
            return null;
        }
        return new String[]{m3u8String, fileName};
    }

    public static long getRunningTasksSize(RequestQueue queue) {
        return queue.getCurrentRequests()
                .stream()
                .filter(r -> r.getDownloaderTask().Status != 7 && r.getDownloaderTask().Status > -1)
                .count();
    }

    public static Intent getVideoActivityIntent(Context context) {
        Intent v = new Intent(context, DownloaderActivity.class);
        v.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        v.putExtra(DownloaderActivity.KEY_UPDATE, true);
        return v;
    }

    public static void renderPauseButton(Context context, ViewHolder viewHolder, DownloaderTask videoTask) {
        viewHolder.button.setImageResource(R.drawable.ic_action_pause);
        viewHolder.button.setOnClickListener(v -> {
            if (!videoTask.IsPaused) {
                videoTask.IsPaused = true;
            } else {
                videoTask.IsPaused = false;
                DownloaderManager.getInstance().getQueue().removeVideoTask(videoTask);
                DownloaderRequest request = new DownloaderRequest(videoTask, DownloaderManager.getInstance(), DownloaderManager.getInstance().getHandler());
                request.setRequestQueue(DownloaderManager.getInstance().getQueue());
                DownloaderManager.getInstance().getQueue().add(request);
            }
        });
    }

    public static File setVideoDownloadDirectory(Context context) {
        File directory =
                //FileShare.isHasSD() ? new File(FileShare.getExternalStoragePath(this), "Videos") :
                context.getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS);
        //Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
        if (!directory.exists()) {
            boolean result = directory.mkdirs();
            if (!result) return null;
        }
        return directory;
    }

    public static void showNotification(Context context, NotificationManager manager, int[] counts) {
        Builder builder = DownloaderHelper.getBuilder(context);
        builder.setContentText(String.format("正在下载 %s/%s 个视频",
                counts[0] - counts[1] + 1,
                counts[0]));
        manager.notify(android.R.drawable.stat_sys_download, builder.build());
    }

    public static void startVideoActivity(Context context) {
        Intent v = new Intent(context, DownloaderActivity.class);
        v.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        context.startActivity(v);
    }

    public static void startVideoListActivity(Context context) {
        Intent intent = new Intent(context, VideoListActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        context.startActivity(intent);
    }

    public static String statusToString(int status) {
        switch (status) {

            case TaskStatus.CREATE_VIDEO_DIRECTORY: {
                return "Create Video Directory";
            }
            case TaskStatus.DOWNLOAD_VIDEOS: {
                return "Download Videos";
            }
            case TaskStatus.PARSE_CONTENT_LENGTH: {
                return "Parse Content Length";
            }
            case TaskStatus.DOWNLOAD_VIDEO_FINISHED: {
                return "Download Video Finished";
            }
            case TaskStatus.MERGE_VIDEO_FINISHED: {
                return "Merge Video Finished";
            }
            case TaskStatus.START: {
                return "Start";
            }
            case TaskStatus.PAUSED: {
                return "Paused";
            }
            case TaskStatus.ERROR_CREATE_DIRECTORY: {
                return "Error Create Directory";
            }
            case TaskStatus.ERROR_CREATE_LOG_FILE: {
                return "Error Create Log File";
            }
            case TaskStatus.ERROR_DOWNLOAD_FILE: {
                return "Error Download File";
            }

            case TaskStatus.ERROR_DELETE_FILE_FAILED: {
                return "Error Delete File Failed";
            }
            case TaskStatus.ERROR_STATUS_CODE: {
                return "Error Status Code";
            }
            default:
                return Integer.toString(status);
        }
    }


    public static void updateList(View progressBar, View listView, DownloaderAdapter videoAdapter) {
        progressBar.setVisibility(View.GONE);
        listView.setVisibility(View.VISIBLE);
        videoAdapter.update(DownloaderManager.getInstance().getQueue().getVideoTasks());
    }
}
