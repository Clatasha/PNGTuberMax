# PNG rig preview

This feature bends a 24 by 24 triangle mesh over a PNG. It also works after the existing plush projection. There are no 3D model files.

1. Open the character layer > PNG bone rig > Create spine / neck / head.
2. With Edit bones in preview enabled, drag the blue endpoints. Adjust each influence radius and strength.
3. Disable editing; try Test lean / nod / turn, then Neutral test pose.
4. Open Preferences > Webcam rig, choose a physical webcam, Start webcam, look forward and Calibrate neutral pose.
5. Enable Left eye, Right eye and/or Mouth in the character's Live facial layers. Position and resize each cutout. These sublayers follow the PNG rig.
6. To attach a separate PNG accessory, select its layer > PNG bone rig > Follow PNG rig, choose the character and place the image normally.
7. Save the project. Bones, attachments, feature placements and webcam input/smoothing are saved. Webcam input starts only when requested; calibration is done per session.

Tracking runs locally in a bundled Python runtime with MediaPipe and OpenCV. No Python installation, separate tracker app or cloud service is required. Camera access follows Windows privacy settings. The virtual camera outputs the final avatar including facial cutouts, without editor bones.

Preview limitations: head rotation drives spine/neck/head influences; there is no arm/body landmark tracking or hierarchical joint solver yet. Facial cutouts use soft elliptical masks; they hide when the plush turns away. They are editable sublayers within their character, rather than independent sprite-manager rows. Manual camera indices are used in this preview. Live webcam behavior must be checked on the user's camera; automated tests cannot verify physical camera hardware.
