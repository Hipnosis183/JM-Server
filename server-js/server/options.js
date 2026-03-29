// Define server options.
export let options = {
  // Allow unregistered users to be registered at the login screen.
  register: undefined,
  // Allow users to have mutiple scores (and replays) in the global rankings.
  // Don't change once the database has already been created.
  multiScores: undefined,
  // Define notice message text.
  // 0. No message text.
  // 1. Custom message text.
  noticeMode: undefined,
  noticeText: undefined,
};

// Initialize server options.
export const optionsInit = () => {
  options = {
    register: 1,
    multiScores: 0,
    noticeMode: 1,
    noticeText: 'Jewelry Master - Server Emulator by Hipnosis.',
  }
}