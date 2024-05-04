<?php
session_start();
include("connect.php");

?>

<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Register & Login</title>
    <link rel="stylesheet" href="style.css">
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.6.0/jquery.min.js"></script>
</head>
<body>
    <div class="container" id="signup" style="display:none;">
    <h1 class="form-title">Register</h1>
    <form method="post" action="register.php">
    <div class="input-group">
       <i class="fas fa-user"></i>
       <input type="text" name="fName" id="fName" placeholder="First Name" required>
       <label for="fName">First Name</label>
    </div>
    <div class="input-group">
        <i class="fas fa-user"></i>
        <input type="text" name="lName" id="lName" placeholder="Last Name" required>
        <label for="lName">Last Name</label>
    </div>
    <div class="input-group">
        <i class="fas fa-envelope"></i>
        <input type="email" name="email" id="signupEmail" placeholder="Email" required>
        <label for="signupEmail">Email</label>
    </div>
    <div class="input-group">
        <i class="fas fa-lock"></i>
        <input type="password" name="password" id="signupPassword" placeholder="Password" required>
        <label for="signupPassword">Password</label>
    </div>
    <input type="submit" class="btn" value="Sign Up" name="signUp">
</form>

      <div class="links">
        <p>Already Have Account ?</p>
        <button id="signInButton">Sign In</button>
      </div>
    </div>
    <div class="container" id="signIn">
        <h1 class="form-title">Sign In</h1>
        <form  method="post" action="register.php">
          <div class="input-group">
              <i class="fas fa-envelope"></i>
              <input type="email" name="email" id="email" placeholder="Email" required>
              <label for="email">Email</label>
          </div>
          <div class="input-group">
              <i class="fas fa-lock"></i>
              <input type="password" name="password" id="password" placeholder="Password" required>
              <label for="password">Password</label>
          </div>
         <input type="button" class="btn" value="Sign In" id="signInBtn" name="signIn">
        </form>
        <div class="links">
          <p>Don't have account yet?</p>
          <button id="signUpButton">Sign Up</button>
        </div>
        <div id="error" style="color: red;"></div>
      </div>
      <script >

$(document).ready(function() {
    $('#signInBtn').click(function(e) {
        e.preventDefault();
        var email = $('#email').val(); // This should correctly fetch the email from the Sign In form
        var password = $('#password').val(); // This should correctly fetch the password from the Sign In form

        $.post('register.php', {
            signIn: true,
            email: email,
            password: password
        }, function(data) {
            if (data === 'success') {
                window.location.href = 'homepage.php';
            } else {
                $('#error').text(data);
                $('#password').val(''); // Clear the password field
            }
        });
    });
});

      </script>
      <script src="script.js"></script>
</body>
</html>
