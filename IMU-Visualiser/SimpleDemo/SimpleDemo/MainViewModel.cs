namespace SimpleDemo
{
    using System;
    using System.Reflection.Metadata.Ecma335;
    using System.Windows.Media;
    using System.Windows.Media.Media3D;
    using HelixToolkit.Wpf;

    // Creates the model for the IMU
    public class MainViewModel
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="MainViewModel"/> class.
        /// </summary>

        private double yaw;
        private double pitch;
        private double roll;

        public double Yaw
        {
            get => yaw;
            set
            {
                yaw = value;
                UpdateTransform();
            }
        }

        public double Pitch
        {
            get => pitch;
            set
            {
                pitch = value;
                UpdateTransform();
            }
        }
        public double Roll
        {
            get => roll;
            set
            {
                roll = value;
                UpdateTransform();
            }
        }

        public Transform3DGroup LinkTransformGroup { get; private set; }
        public MainViewModel()
        {

            // Link Coordinate system with model for transform
            LinkTransformGroup = new Transform3DGroup();

            // Create a model group
            var modelGroup = new Model3DGroup();

            // Mesh Builder represents the MPU6050
            var meshBuilder = new MeshBuilder(false, false);
            meshBuilder.AddBox(new Point3D(0, 0, 0), 4, 4, 1);

            // Create a mesh from the builder (and freeze it)
            var mesh = meshBuilder.ToMesh(true);

            // Creating a material allows us to add the colour to the box
            var blueMaterial = MaterialHelper.CreateMaterial(Colors.Blue);
            //var insideMaterial = MaterialHelper.CreateMaterial(Colors.Yellow);

            modelGroup.Children.Add(new GeometryModel3D { Geometry = mesh, Material = blueMaterial, Transform = LinkTransformGroup });

            // Set the property, which will be bound to the Content property of the ModelVisual3D (see MainWindow.xaml)
            this.Model = modelGroup;
        }

        public Model3D Model { get; set; }

        public void UpdateTransform()
        {
            // Scalar multiplier to amplify the rotation effect
            double scalar = 45.0;

            // Convert yaw, pitch and roll to rads
            double yawRad = Yaw * scalar * (Math.PI / 180);
            double pitchRad = Pitch * scalar * (Math.PI / 180);
            double rollRad = Roll * scalar * (Math.PI / 180);

            // Rotate Transform based on Pitch and Roll
            var yawTransform = new RotateTransform3D(new AxisAngleRotation3D(new Vector3D(0, 0, 1), yawRad));
            var pitchTransform = new RotateTransform3D(new AxisAngleRotation3D(new Vector3D(0, 1, 0), pitchRad));
            var rollTransform = new RotateTransform3D(new AxisAngleRotation3D(new Vector3D(-1, 0, 0), rollRad));

            LinkTransformGroup.Children.Clear();
            LinkTransformGroup.Children.Add(yawTransform);
            LinkTransformGroup.Children.Add(pitchTransform);
            LinkTransformGroup.Children.Add(rollTransform);
        }
    }
}
