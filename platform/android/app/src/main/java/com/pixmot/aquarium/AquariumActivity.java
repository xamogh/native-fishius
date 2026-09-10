package com.pixmot.aquarium;

import android.app.AlertDialog;
import android.os.Bundle;
import android.content.res.AssetManager;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import org.libsdl.app.SDLActivity;

/** SDL owns lifecycle dispatch and touch translation. The Java host only installs
 * bundled resources into private storage for the shared filesystem loader. */
public final class AquariumActivity extends SDLActivity {
    private File resourceDirectory;
    private boolean assetsReady = false;

    @Override protected void onCreate(Bundle savedInstanceState) {
        resourceDirectory = new File(getFilesDir(), "aquarium-assets-v1");
        try {
            copyTree(getAssets(), "", resourceDirectory);
            assetsReady = true;
        } catch (IOException failure) {
            // Do not enter native main with a partially installed resource tree.
            assetsReady = false;
            android.util.Log.e("Aquarium", "Resource installation failed", failure);
        }
        super.onCreate(savedInstanceState);
        if (!assetsReady) {
            new AlertDialog.Builder(this).setTitle("Aquarium could not start")
                .setMessage("The bundled artwork could not be installed in private storage. Free storage and restart the application.")
                .setCancelable(false).setPositiveButton("Close", (dialog, which) -> finish()).show();
        }
    }
    @Override protected String[] getLibraries() {
        return new String[] { "SDL3", "main" };
    }
    @Override protected String[] getArguments() {
        if (!assetsReady) return new String[] { "--assets", "/unavailable-aquarium-resources" };
        return new String[] { "--assets", resourceDirectory.getAbsolutePath(),
            "--save", new File(getFilesDir(), "aquarium-save.json").getAbsolutePath() };
    }
    private static void copyTree(AssetManager manager, String relative, File destination) throws IOException {
        String[] children = manager.list(relative);
        if (children != null && children.length > 0) {
            if (!destination.isDirectory() && !destination.mkdirs()) throw new IOException("Cannot create " + destination);
            for (String child : children) copyTree(manager, relative.isEmpty() ? child : relative + "/" + child, new File(destination, child));
            return;
        }
        File parent = destination.getParentFile();
        if (parent != null && !parent.isDirectory() && !parent.mkdirs()) throw new IOException("Cannot create " + parent);
        File temporary = new File(destination.getPath() + ".installing");
        try (InputStream input = manager.open(relative); FileOutputStream output = new FileOutputStream(temporary)) {
            byte[] buffer = new byte[32768];
            int count;
            while ((count = input.read(buffer)) >= 0) if (count > 0) output.write(buffer, 0, count);
            output.getFD().sync();
        }
        if (destination.exists() && !destination.delete()) throw new IOException("Cannot replace " + destination);
        if (!temporary.renameTo(destination)) throw new IOException("Cannot commit " + destination);
    }
}
