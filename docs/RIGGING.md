# PNG rig preview

This feature bends a PNG mesh and projects a continuous depth surface. The GPU path uses depth testing and supersampling; a continuous CPU raster is retained as a fallback. Body depth comes from a connected distance field rather than independent PNG scanlines. Thin details remain shallow; roundness and face flatness control the volume. There are no 3D model files.

1. Open the character layer > PNG bone rig > Create spine / neck / head.
2. With Edit bones in preview enabled, drag the blue endpoints. Adjust each influence radius and strength.
3. Disable editing; try Test lean / nod / turn, then Neutral test pose.
4. Open Preferences > Webcam rig, choose a physical webcam, Start webcam, look forward and Calibrate neutral pose.
5. Enable Left eye, Right eye and/or Mouth in the character's Live facial layers. Position and resize each cutout. These sublayers are composited onto the PNG before turning and rig deformation. Use Edit crop, zoom and lighting for the large preview; drag to pan and adjust Source zoom / Crop width / Crop height. Avatar X/Y, width, height and rotation are separate placement controls.
6. To attach a separate PNG accessory, select its layer > Attachment > Attach to, choose the character and use the accessory Transform controls to position it. This locks its movement to the parent and bypasses independent motion/physics. The binding saves with the project.
7. Save the project. Bones, attachments, feature placements and webcam input/smoothing are saved. Webcam input starts only when requested; calibration is done per session.

Tracking runs locally in a bundled Python runtime with MediaPipe and OpenCV. No Python installation, separate tracker app or cloud service is required. Camera access follows Windows privacy settings. The virtual camera outputs the final avatar including facial cutouts, without editor bones.

Preview limitations: head rotation drives spine/neck/head influences; there is no arm/body landmark tracking or hierarchical joint solver yet. Facial features use tracked eyelid/lip contours or simple soft-edged masks, and turn/occlude as part of the PNG surface. They are editable sublayers within their character, rather than independent sprite-manager rows. Live webcam behavior must be checked on the user's camera; automated tests cannot verify physical camera hardware.

## Precise eyes and lips
In each crop editor choose Use precise eyelid / lip mask (or Mask shape > Tracked eyelids / lips). Existing saved projects retain their previous mask until selected. Start with 1 to 3 px Contour padding for eyelids and 0 to 1 px for lips, then adjust Contour feather. Show source and mask boundary overlays the edge for alignment; the overlay appears only in the editor. Offset and width/height corrections adjust the tracked boundary independently of source cropping. Mouth interior remains included when talking. Copy settings also copies these corrections. Replace the app and tracker.py together to update the contour transport.

## Level webcam features
The tracker levels both eye crops and the mouth with one shared roll measured across the eyes. Camera pixels and contour masks use the same transform, and crop dimensions are measured in that level basis. Tilting your physical head no longer independently tips or shrinks each source cutout. This stabilization is automatic. Manual crop/placement rotation remains available; the character's rig still moves the attached facial surface.

## Locked accessories
Attachment works with flat PNGs and plush characters, with or without a bone rig. A front accessory follows the parent surface during turning and hides when the rear faces the viewer. Put the accessory above its mascot in the sprite manager when it should draw on top. Separate outfit layers can use the existing states/visibility controls. Rename Layer has a full-width Save button below its color and tag controls; Enter in the name field leaves the editor open.
