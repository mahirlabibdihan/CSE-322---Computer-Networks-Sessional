package io;

import java.nio.file.*;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;
import java.lang.String;

public class Console {
    private static Scanner sc = new Scanner(System.in);

    public static int getOption(int start, int end) {
        while (true) {
            System.out.print(Color.BLUE + "Select an Option: " + Color.RESET);
            int option = getNonNegativeInt();
            if (option >= start && option <= end) {
                return option;
            } else if (option != -1) {
                printError("Invalid option");
            }
        }
    }

    public static String getFilePath() {
        while (true) {
            String file = Console.getLine("Enter File Path");
            Path path = Paths.get(file);
            if (Files.exists(path)) {
                return file;
            } else {
                printError("Invalid path");
            }
        }
    }

    public static String getString(String msg) {
        System.out.print(Color.YELLOW + msg + ": " + Color.RESET);
        String temp = sc.next();
        sc.nextLine();
        return temp;
    }

    public static String getString() {
        String temp = sc.next();
        sc.nextLine();
        return temp;
    }

    public static String getLine() {
        return sc.nextLine().trim();
    }

    public static String getLine(String msg) {
        System.out.print(Color.YELLOW + msg + ": " + Color.RESET);
        return sc.nextLine().trim();
    }

    public static int getRangeInt(String msg, int min, int max) {
        while (true) {
            System.out.print(Color.YELLOW + msg + Color.RESET);
            int input = getNonNegativeInt();
            if (input != -1) {
                if (input >= min && input <= max) {
                    return input;
                } else {
                    printError("Input must be between " + min + " and " + max);
                }
            }
        }
    }

    public static int getPositiveInt(String msg) {
        while (true) {
            System.out.print(Color.BLUE + msg + Color.RESET);
            int input = getPositiveInt();
            if (input != -1) {
                return input;
            }
        }
    }

    public static int getPositiveInt() {
        try {
            int temp = sc.nextInt();
            sc.nextLine();
            if (temp <= 0) {
                printError("Must be a Positive integer");
                return -1;
            }
            return temp;
        } catch (Exception e) {
            printError("Must be an integer");
            sc.nextLine();
            return -1;
        }
    }

    public static int getNonNegativeInt() {
        try {
            int temp = sc.nextInt();
            sc.nextLine();
            if (temp < 0) {
                printError("Must be a Non-Negative integer");
                return -1;
            }
            return temp;
        } catch (Exception e) {
            printError("Must be an integer");
            sc.nextLine();
            return -1;
        }
    }

    public static void printError(String error) {
        System.out.print(Color.RED);
        System.out.print("⚠ Error: " + error);
        System.out.println(Color.RESET);
    }

    public static void printSuccess(String message) {
        System.out.print(Color.GREEN);
        System.out.print("Success: " + message);
        System.out.println(Color.RESET);
    }

    public static <T> void printList(List<T> list) {
        if (list.isEmpty()) {
            printError("Nothing to show");
        }
        for (int i = 0; i < list.size(); i++) {
            System.out.println((i + 1) + ". " + list.get(i));
        }
    }

    public static void printWarning(String message) {
        System.out.print(Color.YELLOW);
        System.out.print("Warning: " + message);
        System.out.println(Color.RESET);
    }

    public static void closeScanner() {
        sc.close();
    }

    // Stop taking further input until user presses Enter
    public static void pauseScanner() {
        System.out.print("\n" + Color.YELLOW + "Press enter to continue...." + Color.RESET);
        sc.nextLine();
        System.out.println("");
    }
}